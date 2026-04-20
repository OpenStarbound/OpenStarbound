/*
	Helper fonctions for Streaming textures
*/
#ifdef TEXSTREAM

#include <bc_cat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "streaming.h"

#include "../gl/loader.h"
#include "../gl/gles.h"

PFNGLTEXBINDSTREAMIMGPROC *glTexBindStreamIMG = NULL;
PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC *glGetTexAttrIMG = NULL;
PFNGLGETTEXSTREAMDEVICENAMEIMGPROC *glGetTexDeviceIMG = NULL;

//extern void* eglGetProcAddress(const char*);

int gl_streaming = 0;
int gl_streaming_initialized = 0;
int bc_cat[10];
int tex_free[10];
const GLubyte * bcdev[10];
int bcdev_w, bcdev_h, bcdev_n;
int bcdev_fmt;
unsigned long buf_paddr[10];    // physical address
char *buf_vaddr[10];            // virtual address

void Streaming_Initialize() {
    LOAD_EGL(eglGetProcAddress);
	if (gl_streaming_initialized)
		return;
	// get the extension functions
	gl_streaming_initialized = 1;
    glTexBindStreamIMG =(PFNGLTEXBINDSTREAMIMGPROC*)egl_eglGetProcAddress("glTexBindStreamIMG");
    glGetTexAttrIMG = (PFNGLGETTEXSTREAMDEVICEATTRIBUTEIVIMGPROC*)egl_eglGetProcAddress("glGetTexStreamDeviceAttributeivIMG");
    glGetTexDeviceIMG = (PFNGLGETTEXSTREAMDEVICENAMEIMGPROC*)egl_eglGetProcAddress("glGetTexStreamDeviceNameIMG");

	if (!glTexBindStreamIMG || !glGetTexAttrIMG || !glGetTexDeviceIMG) {
		gl_streaming = 0;
		return;
	}
	gl_streaming = 1;
	// initialise the bc_cat ids
	for (int i=0; i<10; i++) {
		bc_cat[i] = -1;
		tex_free[i] = 1;
	}
}

int open_bccat(int i) {
	if (bc_cat[i]>-1)
		return bc_cat[i];
	char buff[]="/dev/bccat0";
	buff[strlen(buff)-1]='0'+i;
	bc_cat[i] = open(buff, O_RDWR|O_NDELAY);
    return bc_cat[i];
}
void close_bccat(int i) {
    if (bc_cat[i]==-1)
        return;
    close(bc_cat[i]);
    bc_cat[i]=-1;
    return;
}

int alloc_buff(int buff, int width, int height) {
	if (!gl_streaming_initialized)
		Streaming_Initialize;
	if (!gl_streaming)
		return 0;
	if ((buff<0) || (buff>9))
		return 0;
	if (!tex_free[buff])
		return 0;
	if (open_bccat(buff)<0)
		return 0;
    BCIO_package ioctl_var;
    bc_buf_params_t buf_param;
	buf_param.count = 1;	// only 1 buffer?
	buf_param.width = width;
	buf_param.height = height;
	buf_param.fourcc = BC_PIX_FMT_RGB565;	// only RGB565 here (other choices are only some YUV formats)
	buf_param.type = BC_MEMORY_MMAP;
	if (ioctl(bc_cat[buff], BCIOREQ_BUFFERS, &buf_param) != 0) {
		printf("LIBGL: BCIOREQ_BUFFERS failed\n");
		return 0;
	}
	if (ioctl(bc_cat[buff], BCIOGET_BUFFERCOUNT, &ioctl_var) != 0) {
		printf("LIBGL: BCIOREQ_BUFFERCOUNT failed\n");
		return 0;
	}
	if (ioctl_var.output == 0) {
		printf("LIBGL: Streaming, no texture buffer available\n");
		return 0;
	}
	const char *bcdev = glGetTexDeviceIMG(buff);
	if (!bcdev) {
		printf("LIBGL: problem with getting the GL_IMG_texture_stream device\n");
		return 0;
	} else {
		bcdev_w = width;
		bcdev_h = height;
		bcdev_n = 1;
		glGetTexAttrIMG(buff, GL_TEXTURE_STREAM_DEVICE_NUM_BUFFERS_IMG, &bcdev_n);
		glGetTexAttrIMG(buff, GL_TEXTURE_STREAM_DEVICE_WIDTH_IMG, &bcdev_w);
		glGetTexAttrIMG(buff, GL_TEXTURE_STREAM_DEVICE_HEIGHT_IMG, &bcdev_h);
		glGetTexAttrIMG(buff, GL_TEXTURE_STREAM_DEVICE_FORMAT_IMG, &bcdev_fmt);
		printf("LIBGL: Streaming device = %s num: %d, width: %d, height: %d, format: 0x%x\n",
			bcdev, bcdev_n, bcdev_w, bcdev_h, bcdev_fmt);
		if (bcdev_w!=width) {
			printf("LIBGL: Streaming not activate, buffer width != asked width\n");
			return 0;
		}
	}
/*	LOAD_GLES(glTexParameterf);
    gles_glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gles_glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
	
	ioctl_var.input = 0;
	if (ioctl(bc_cat[buff], BCIOGET_BUFFERPHYADDR, &ioctl_var) != 0) {
		printf("LIBGL: BCIOGET_BUFFERADDR failed\n");
		return 0;
	} else {
		buf_paddr[buff] = ioctl_var.output;
		buf_vaddr[buff] = (char *)mmap(NULL, width*height*2,
						  PROT_READ | PROT_WRITE, MAP_SHARED,
						  bc_cat[buff], buf_paddr[buff]);

		if (buf_vaddr[buff] == MAP_FAILED) {
			printf("LIBGL: mmap failed\n");
			return 0;
		}
	}
	
	// All done!
	tex_free[buff] = 0;
	return 1;
}

int free_buff(int buff) {
	if (!gl_streaming)
		return 0;
	if ((buff<0) || (buff>9))
		return 0;
    close_bccat(buff);
	tex_free[buff] = 1;
	return 1;
}


/* 
	Streaming Cache functions
*/

int streaming_inited = 0;

typedef struct {
	int	active;
	unsigned int last;	// to get the age of last update
	unsigned int texID;	// ID of texture
} glstreaming_t;
glstreaming_t stream_cache[10];
unsigned int frame_number;

// Function to start the Streaming texture Cache
int InitStreamingCache() {
//printf("InitStreamingCache\n");
	if (streaming_inited)
		return gl_streaming;
	Streaming_Initialize();
	for (int i=0; i<10; i++) {
		stream_cache[i].active = 0;
		stream_cache[i].last = 0;
		stream_cache[i].texID = 0;
	}
	frame_number = 0;
	streaming_inited = 1;
	return gl_streaming;
}

// Function to get a Streaming buffer address
void* GetStreamingBuffer(int buff) {
//printf("GetStreamingBuffer(%i)\n", buff);
	if (!gl_streaming)
		return NULL;
	if ((buff<0) || (buff>9))
		return NULL;
	if (tex_free[buff])
		return NULL;
	stream_cache[buff].last = frame_number;
	return buf_vaddr[buff];
}

// Function to add a new texture of size Width*Height, with fake Texture ID "ID". Return the ID or -1 if failed.
int AddStreamed(int width, int height, unsigned int ID) {
//printf("AddStreamed(%i, %i, %u)\n", width, height, ID);
	if (!gl_streaming)
		return -1;
	static int i =0;
    int j=0;
	while (j<10) {
        int k = (i+j)%10;
		if (tex_free[k]) {
			if (alloc_buff(k, width, height)) {
				stream_cache[k].active = 1;
				stream_cache[k].last = frame_number;
				stream_cache[k].texID = ID;
                i = (i+j+1)%10;
				return k;
			} else {                
                return -1;	// Probably useless to try again and again
            }
		}
        j++;
	}
	return -1;
}

// Function to free a streamed texture ID
void FreeStreamed(int ID) {
//printf("FreeStreamed(%i)", ID);
	if (!gl_streaming)
		return;
	if ((ID<0) || (ID>9))
		return;
	if (tex_free[ID])
		return;
	if (!stream_cache[ID].active)
		return;
		
	free_buff(ID);
	stream_cache[ID].active = 0;
	stream_cache[ID].texID = 0;
}

// Function to find a StreamingID with a TextureID. G-1 if not found
int FindTexID(unsigned int ID) {
//printf("FindTexID(%u)\n", ID);
	if (!gl_streaming)
		return -1;
	int i =0;
	while (i<10) {
		if (stream_cache[i].texID==ID)
			return i;
		i++;
	}
	return -1;
}

// Function to apply Min& Mag filter to Streaming texture
void ApplyFilterID(int ID, GLenum min_filter, GLenum mag_filter) {
//printf("ApplyFilterID(%i, 0x%04X, 0x%04X)\n", ID, min_filter, mag_filter);
	if (!gl_streaming)
		return;
	if ((ID<0) || (ID>9))
		return;
	if (tex_free[ID])
		return;
	if (!stream_cache[ID].active)
		return;
    gl4es_glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MIN_FILTER, min_filter);
    gl4es_glTexParameterf(GL_TEXTURE_STREAM_IMG, GL_TEXTURE_MAG_FILTER, mag_filter);
}

// Function to activate the Steaming texture ID on current tex...
void ActivateStreaming(int ID) {
//printf("ActivateStreaming(%i)\n", ID);
    LOAD_GLES(glEnable);
	if (!gl_streaming)
		return;
	if ((ID<0) || (ID>9))
		return;
	if (tex_free[ID])
		return;
	if (!stream_cache[ID].active)
		return;

//	gles_glEnable(GL_TEXTURE_STREAM_IMG);
	glTexBindStreamIMG(ID, 0);
}

// Function to deactivate the Streaming texture on current tex...
void DeactivateStreaming() {
//printf("DeactivateStreaming()\n");
    LOAD_GLES(glDisable);
	if (!gl_streaming)
		return;
//	gles_glDisable(GL_TEXTURE_STREAM_IMG);
}
#endif  //TEXSTREAM
