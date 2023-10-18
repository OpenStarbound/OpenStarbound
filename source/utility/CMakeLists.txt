INCLUDE_DIRECTORIES (
    ${STAR_EXTERN_INCLUDES}
    ${STAR_CORE_INCLUDES}
    ${STAR_BASE_INCLUDES}
    ${STAR_PLATFORM_INCLUDES}
    ${STAR_GAME_INCLUDES}
  )

ADD_EXECUTABLE (asset_packer
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base>
  asset_packer.cpp)
TARGET_LINK_LIBRARIES (asset_packer ${STAR_EXT_LIBS})

ADD_EXECUTABLE (asset_unpacker
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base>
  asset_unpacker.cpp)
TARGET_LINK_LIBRARIES (asset_unpacker ${STAR_EXT_LIBS})

ADD_EXECUTABLE (btree_repacker
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base>
  btree_repacker.cpp)
TARGET_LINK_LIBRARIES (btree_repacker ${STAR_EXT_LIBS})

ADD_EXECUTABLE (dump_versioned_json
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  dump_versioned_json.cpp)
TARGET_LINK_LIBRARIES (dump_versioned_json ${STAR_EXT_LIBS})

ADD_EXECUTABLE (game_repl
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  game_repl.cpp)
TARGET_LINK_LIBRARIES (game_repl ${STAR_EXT_LIBS})

ADD_EXECUTABLE (make_versioned_json
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  make_versioned_json.cpp)
TARGET_LINK_LIBRARIES (make_versioned_json ${STAR_EXT_LIBS})

ADD_EXECUTABLE (planet_mapgen
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  planet_mapgen.cpp)
TARGET_LINK_LIBRARIES (planet_mapgen ${STAR_EXT_LIBS})

ADD_EXECUTABLE (render_terrain_selector
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  render_terrain_selector.cpp)
TARGET_LINK_LIBRARIES (render_terrain_selector ${STAR_EXT_LIBS})

ADD_EXECUTABLE (update_tilesets
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  update_tilesets.cpp tileset_updater.cpp)
TARGET_LINK_LIBRARIES (update_tilesets ${STAR_EXT_LIBS})

ADD_EXECUTABLE (fix_embedded_tilesets
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  fix_embedded_tilesets.cpp)
TARGET_LINK_LIBRARIES (fix_embedded_tilesets ${STAR_EXT_LIBS})

ADD_EXECUTABLE (world_benchmark
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  world_benchmark.cpp)
TARGET_LINK_LIBRARIES (world_benchmark ${STAR_EXT_LIBS})

ADD_EXECUTABLE (generation_benchmark
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  generation_benchmark.cpp)
TARGET_LINK_LIBRARIES (generation_benchmark ${STAR_EXT_LIBS})

ADD_EXECUTABLE (dungeon_generation_benchmark
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  dungeon_generation_benchmark.cpp)
TARGET_LINK_LIBRARIES (dungeon_generation_benchmark ${STAR_EXT_LIBS})

ADD_EXECUTABLE (map_grep
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  map_grep.cpp)
TARGET_LINK_LIBRARIES (map_grep ${STAR_EXT_LIBS})

ADD_EXECUTABLE (word_count
  $<TARGET_OBJECTS:star_extern> $<TARGET_OBJECTS:star_core> $<TARGET_OBJECTS:star_base> $<TARGET_OBJECTS:star_game>
  word_count.cpp)
TARGET_LINK_LIBRARIES (word_count ${STAR_EXT_LIBS})
