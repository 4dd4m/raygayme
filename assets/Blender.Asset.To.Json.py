import bpy
import json
import os
import struct
import uuid
from mathutils import Vector

objects = []
terrain_chunks = []
HEIGHTMAP_TARGET_CELL_SIZE = 0.5
HEIGHTMAP_RAY_MARGIN = 1000.0
HEIGHTMAP_EDGE_SAMPLE_EPSILON = 0.001

assetId = [
    "TERRAIN",
    "ASSET_TREE",
    "ASSET_TREE_TRUNK",
    "ASSET_ROCK",
    "COLLISION_TREE"
]

#--------------------------------------------------
# Export terrain chunks for server-side terrain queries
#--------------------------------------------------


def to_game_coords(blender_vec):
    return [blender_vec.x, blender_vec.z, -blender_vec.y]


def to_blender_coords(game_x, game_y, game_z):
    return Vector((game_x, -game_z, game_y))


def get_chunk_coord(obj):
    parts = obj.name.split("_")
    if obj.name.startswith("Chunk") and len(parts) >= 3:
        return [int(parts[-2]), int(parts[-1])]

    return None


def get_chunk_file_stem(obj):
    coord = get_chunk_coord(obj)
    if coord is None:
        return None
    return f"Chunk{coord[0]}_{coord[1]}"


def get_chunk_model_file_name(obj):
    coord = get_chunk_coord(obj)
    if coord is None:
        return None
    return f"CHUNK_{coord[0]}_{coord[1]}.glb"


def sample_terrain_height(evaluated_obj, game_x, game_z, min_game_y, max_game_y):
    world_origin = to_blender_coords(game_x, max_game_y + HEIGHTMAP_RAY_MARGIN, game_z)
    world_direction = Vector((0.0, 0.0, -1.0))
    inverse_matrix = evaluated_obj.matrix_world.inverted()
    local_origin = inverse_matrix @ world_origin
    local_direction = (inverse_matrix.to_3x3() @ world_direction).normalized()
    ray_distance = (max_game_y - min_game_y) + HEIGHTMAP_RAY_MARGIN * 2.0

    hit, local_location, _normal, _face_index = evaluated_obj.ray_cast(
        local_origin,
        local_direction,
        distance=ray_distance
    )

    if not hit:
        return None

    world_location = evaluated_obj.matrix_world @ local_location
    return to_game_coords(world_location)[1]


def nearest_vertex_height(verts, game_x, game_z):
    nearest_height = 0.0
    nearest_distance_sq = None

    for vert in verts:
        dx = vert[0] - game_x
        dz = vert[2] - game_z
        distance_sq = dx * dx + dz * dz

        if nearest_distance_sq is None or distance_sq < nearest_distance_sq:
            nearest_distance_sq = distance_sq
            nearest_height = vert[1]

    return nearest_height


def select_object_hierarchy(obj):
    obj.select_set(True)
    for child in obj.children:
        select_object_hierarchy(child)


def deselect_scene_objects():
    for obj in bpy.context.scene.objects:
        obj.select_set(False)


def get_selected_mesh_count():
    return sum(1 for selected in bpy.context.selected_objects if selected.type == "MESH")


def export_terrain_chunk(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated_obj = obj.evaluated_get(depsgraph)
    mesh = evaluated_obj.to_mesh()

    try:
        mesh.calc_loop_triangles()

        verts = []

        for vertex in mesh.vertices:
            world_pos = evaluated_obj.matrix_world @ vertex.co
            game_pos = to_game_coords(world_pos)
            verts.append(game_pos)

        min_bounds = [
            min(v[0] for v in verts),
            min(v[1] for v in verts),
            min(v[2] for v in verts),
        ]
        max_bounds = [
            max(v[0] for v in verts),
            max(v[1] for v in verts),
            max(v[2] for v in verts),
        ]

        width = max_bounds[0] - min_bounds[0]
        depth = max_bounds[2] - min_bounds[2]
        grid_width = max(2, int(round(width / HEIGHTMAP_TARGET_CELL_SIZE)) + 1)
        grid_depth = max(2, int(round(depth / HEIGHTMAP_TARGET_CELL_SIZE)) + 1)
        cell_size_x = width / (grid_width - 1)
        cell_size_z = depth / (grid_depth - 1)

        heights = []
        missed_samples = 0
        for z_index in range(grid_depth):
            z = min_bounds[2] + z_index * cell_size_z
            for x_index in range(grid_width):
                x = min_bounds[0] + x_index * cell_size_x
                sample_x = x
                sample_z = z

                if x_index == 0:
                    sample_x += HEIGHTMAP_EDGE_SAMPLE_EPSILON
                elif x_index == grid_width - 1:
                    sample_x -= HEIGHTMAP_EDGE_SAMPLE_EPSILON

                if z_index == 0:
                    sample_z += HEIGHTMAP_EDGE_SAMPLE_EPSILON
                elif z_index == grid_depth - 1:
                    sample_z -= HEIGHTMAP_EDGE_SAMPLE_EPSILON

                height = sample_terrain_height(evaluated_obj, sample_x, sample_z, min_bounds[1], max_bounds[1])

                if height is None:
                    missed_samples += 1
                    height = nearest_vertex_height(verts, x, z)

                heights.append(float(height))

        base_output_path = os.path.dirname(bpy.data.filepath)
        heightmaps_directory = os.path.join(base_output_path, "heightmaps")
        os.makedirs(heightmaps_directory, exist_ok=True)

        chunk_file_stem = get_chunk_file_stem(obj)
        if chunk_file_stem is None:
            print(f"Skipped terrain chunk {obj.name}: expected name Chunk_x_z")
            return None
        height_file_name = f"{chunk_file_stem}.height"
        json_file_name = f"{chunk_file_stem}.json"
        height_output_path = os.path.join(heightmaps_directory, height_file_name)
        json_output_path = os.path.join(heightmaps_directory, json_file_name)

        with open(height_output_path, "wb") as f:
            for height in heights:
                f.write(struct.pack("<f", height))

        height_map_data = {
            "id": obj.name,
            "coord": get_chunk_coord(obj),
            "heightFormat": "float32-le",
            "heightOrder": "z-major: heights[z * gridWidth + x]",
            "origin": [min_bounds[0], min_bounds[1], min_bounds[2]],
            "gridWidth": grid_width,
            "gridDepth": grid_depth,
            "bounds": {
                "min": min_bounds,
                "max": max_bounds,
            }
        }

        with open(json_output_path, "w", encoding="utf-8") as f:
            json.dump(height_map_data, f, indent=4)

        if missed_samples > 0:
            print(f"Terrain chunk '{obj.name}' used nearest vertex fallback for {missed_samples} height samples")

        print(f"Exported terrain chunk '{obj.name}' to '{json_output_path}' and '{height_output_path}'")
        return height_map_data
    finally:
        evaluated_obj.to_mesh_clear()


#--------------------------------------------------
# Export Every Static Object Position and rotation
#--------------------------------------------------


for obj in bpy.context.scene.objects:
    if obj.get("blenderOnly") is None:
        raise TypeError(f"{obj.name} has no blenderOnly property")

    if obj.get("blenderOnly") is True:
        continue

    if obj.name.startswith("Chunk"):
        terrain_chunks.append(export_terrain_chunk(obj))
        continue

    data = obj.name.split("|")

    if obj.get("id") is None:
        obj["id"] = str(uuid.uuid4())

    if obj.get("interactive") is None:
        raise TypeError(f"interactive property is not set on: {obj.name}")

    if obj.get("type") is None:
        raise TypeError(f"{obj.name} has no type")

    if obj.get("chunk") is None:
        raise TypeError(f"{obj.name} has no chunkId")

    if obj.get("type") not in assetId:
        raise TypeError(f"{obj.name} - wrong type or missing assetId")

    objects.append({
        "id": obj.get("id"),
        "interactive": obj.get("interactive"),
        "name": obj.name,
        "type": assetId.index(obj.get("type")),
        "chunk": obj.get("chunk"),
        "position": [
            obj.location.x,
            obj.location.z,
            -obj.location.y
        ],
        "rotation": [
            obj.rotation_euler.x,
            obj.rotation_euler.z,
            -obj.rotation_euler.y
        ],
        "scale": [
            obj.scale.x,
            obj.scale.z,
            obj.scale.y
        ]
    })

base_output_path = os.path.dirname(bpy.data.filepath)

objects_output_path = os.path.join(
    base_output_path,
    "objects.json"
)

with open(objects_output_path, "w", encoding="utf-8") as f:
    if len(objects) == 0:
        f.write("{}")
    else:
        json.dump({"objects": objects}, f, indent=4)

print(f"Exported {len(objects)} objects to {objects_output_path}")

print(f"Exported {len(terrain_chunks)} terrain chunks")

#------------------------------------------------------------------
# Export one of each object
#------------------------------------------------------------------

exported_types = set()

for obj in bpy.context.scene.objects:
    if obj.get("blenderOnly") is True:
        continue

    object_type = obj.get("type")

    if object_type is None:
        raise TypeError(f"{obj.name} has no type property")

    if object_type.startswith("TERRAIN"):
        continue

    # One export per unique type
    if object_type in exported_types:
        continue

    if object_type.startswith("ASSET"):
        folder_name = "assets"
    elif object_type.startswith("COLLISION"):
        folder_name = "collisions"
    else:
        folder_name = "other"

    output_directory = os.path.join(
        base_output_path,
        folder_name
    )

    os.makedirs(output_directory, exist_ok=True)

    output_path = os.path.join(
        output_directory,
        f"{object_type}.glb"
    )

    deselect_scene_objects()

    select_object_hierarchy(obj)
    bpy.context.view_layer.objects.active = obj
    selected_names = [selected.name for selected in bpy.context.selected_objects]
    selected_mesh_count = get_selected_mesh_count()

    print(
        f"Preparing asset export '{object_type}' from '{obj.name}': "
        f"selected {len(selected_names)} object(s), {selected_mesh_count} mesh object(s): {selected_names}"
    )

    original_location = obj.location.copy()
    original_rotation = obj.rotation_euler.copy()
    original_scale = obj.scale.copy()

    try:
        if selected_mesh_count == 0:
            print(f"WARNING: Asset '{object_type}' has no selected mesh objects before export")

        # Export the model around the world origin
        obj.location = (0.0, 0.0, 0.0)
        obj.rotation_euler = (0.0, 0.0, 0.0)
        obj.scale = (1.0, 1.0, 1.0)

        # Ensure Blender updates the transform
        bpy.context.view_layer.update()

        bpy.ops.export_scene.gltf(
            filepath=output_path,
            export_format="GLB",
            use_selection=True,
            export_apply=True,
            export_yup=True,
            export_texcoords=True,
            export_normals=True,
            export_materials="EXPORT"
        )

        exported_types.add(object_type)
        output_size = os.path.getsize(output_path) if os.path.exists(output_path) else 0

        print(
            f"Exported type '{object_type}' "
            f"from object '{obj.name}' to '{output_path}' ({output_size} bytes)"
        )

    finally:
        obj.location = original_location
        obj.rotation_euler = original_rotation
        obj.scale = original_scale
        bpy.context.view_layer.update()

print(f"Exported {len(exported_types)} unique object types")
#------------------------------------------------------------------
# Export every terrain chunk
#------------------------------------------------------------------

exported_chunk_count = 0

for obj in bpy.context.scene.objects:
    if obj.get("blenderOnly") is True:
        continue

    object_type = obj.get("type")

    if object_type is None:
        continue

    if not object_type.startswith("TERRAIN"):
        continue

    output_directory = os.path.join(base_output_path, "chunks")
    os.makedirs(output_directory, exist_ok=True)

    chunk_model_file_name = get_chunk_model_file_name(obj)
    if chunk_model_file_name is None:
        print(f"Skipped terrain chunk {obj.name}: expected name Chunk_x_z")
        continue

    output_path = os.path.join(output_directory, chunk_model_file_name)

    deselect_scene_objects()

    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    bpy.context.view_layer.update()

    bpy.ops.export_scene.gltf(
        filepath=output_path,
        export_format="GLB",
        use_selection=True,
        export_apply=True,
        export_yup=True,
        export_texcoords=True,
        export_normals=True,
        export_materials="EXPORT"
    )

    exported_chunk_count += 1

    print(
        f"Exported terrain chunk '{obj.name}' "
        f"to '{output_path}'"
    )

print(f"Exported {exported_chunk_count} terrain chunk models")
bpy.ops.wm.save_mainfile()






