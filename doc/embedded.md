Some asset paths can have data embedded in them.
This behaviour can be globally disabled through `safe.allowRemoteAssets`. It is enabled by default.

Some things are specifically excluded from being remote:
    - Scripts
    - Everything using assets.bytes or root.assetData, by default (to prevent `loadstring` on a remote script)

Every other asset is allowed. This includes:
    - Anything json, including animation configs
    - Sounds
    - Images
        - Images can have frames data embedded in them as well.

For images, this system should be far more efficient than equivalent directives-based approaches.
For sounds and path-only json, this system is the only way to include custom assets without needing a mod.

This system is not vanilla-compatible! Trying to use it where it is unsupported will cause crashes and other issues.

Some utility callbacks are provided for in-game conversion to supported data.
`sb.embedData`
`sb.embedJson`
`sb.embedText`
`Image:embed`

If you want to build embedded paths yourself, here's how they work:

An asset path starting with `/opensb_data;` is treated as an embedded asset.
Embedded assets are formatted in semicolon-separated parts.
The first part is the aforementioned indicator.

Then, there's a format:
`text` indicates escaped plaintext. Swap `:` for `%c`, `?` for `%q`, `;` for `%s`, `/` for `%b`. Prevent undesired escapes by swapping `%` for `%%`.
Useable for any text file.
```
/opensb_data;text;this is text data%s it exists.
```
`bin` indicates raw binary data, directly equivalent to there being a file at this path.
The binary data is stored encoded as **Base64URL**. (Regular Base64 can and will cause crashes on unsupported clients due to asset path errors.)
```
/opensb_data;bin;T2dnUwACAAAAAAAAAAABAAAAAA...
```
Compression methods can also be specified, where the encoded data is decompressed before usage.
Available methods are `zstd`, `gzip`, `zlib`, and `none`. The data is uncompressed by default.
Useable for any kind of file. JSON data in this format is treated as plaintext.
    `image` is similar to `bin` but allows attaching frames data to the end:
        ```
        /opensb_data;bin;T2dnUwACAAAAAAAAAAABAA...Dg==;framespath;/humanoid/frontarm.frames
        /opensb_data;bin;T2dnUwACAAAAAAAAAAABAA...Dg==;framespath;text;{"frameGrid"%c{"size"%c[43,43],"dimensions"%c[1,1],"names"%c[["h"]]}}
        /opensb_data;bin;T2dnUwACAAAAAAAAAAABAA...Dg==;framespath;bin;eyJmcmFtZUdyaWQiOnsic2l6ZSI6WzQzLDQzXSwiZGltZW5zaW9ucyI6WzEsMV0sIm5hbWVzIjpb
WyJoIl1dfX0=
        ```
    `json` is similar to `bin`, but encodes in Starbound's binary JSON format, rather than plaintext JSON.
    
    
For best efficiency, use `zstd` compression for large amounts of binary data and minimize whitespace in stored plaintext.



Make sure to use Base64URL for your data to reduce crashes from unsupported clients! (Replace `/` in Base64 data with `_` and `+` with `-`)
https://base64.guru/standards/base64url
While the game will still parse Base64, the slashes in Base64 data will crash older/vanilla clients.
