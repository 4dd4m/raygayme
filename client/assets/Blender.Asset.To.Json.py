import bpy
import json
import os
import uuid

objects = []
terrain_chunks = []

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


def get_chunk_coord(obj):
    chunk = obj.get("chunk")
    if chunk is not None:
        if isinstance(chunk, str):
            parts = chunk.replace(",", "_").split("_")
            if len(parts) >= 2:
                return [int(parts[-2]), int(parts[-1])]
            return [int(chunk), 0]
        try:
            return [int(chunk[0]), int(chunk[1])]
        except TypeError:
            return [int(chunk), 0]

    parts = obj.name.split("_")
    if len(parts) >= 3:
        return [int(parts[-2]), int(parts[-1])]

    return [0, 0]


def export_terrain_chunk(obj):
    depsgraph = bpy.context.evaluated_depsgraph_get()
    evaluated_obj = obj.evaluated_get(depsgraph)
    mesh = evaluated_obj.to_mesh()

    try:
        mesh.calc_loop_triangles()

        verts = []
        heights_by_xz = {}
        xs = []
        zs = []

        for vertex in mesh.vertices:
            world_pos = evaluated_obj.matrix_world @ vertex.co
            game_pos = to_game_coords(world_pos)
            verts.append(game_pos)

            x_key = round(game_pos[0], 5)
            z_key = round(game_pos[2], 5)
            heights_by_xz[(x_key, z_key)] = game_pos[1]
            xs.append(x_key)
            zs.append(z_key)

        tris = [[tri.vertices[0], tri.vertices[1], tri.vertices[2]] for tri in mesh.loop_triangles]

        unique_xs = sorted(set(xs))
        unique_zs = sorted(set(zs))
        grid_width = len(unique_xs)
        grid_depth = len(unique_zs)

        heights = []
        for z in unique_zs:
            for x in unique_xs:
                heights.append(heights_by_xz.get((x, z), 0.0))

        cell_size_x = unique_xs[1] - unique_xs[0] if grid_width > 1 else 0.0
        cell_size_z = unique_zs[1] - unique_zs[0] if grid_depth > 1 else 0.0
        cell_size = cell_size_x if abs(cell_size_x - cell_size_z) < 0.0001 else [cell_size_x, cell_size_z]

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

        grid_size = grid_width if grid_width == grid_depth else [grid_width, grid_depth]

        return {
            "id": obj.name,
            "coord": get_chunk_coord(obj),
            "origin": [min_bounds[0], min_bounds[1], min_bounds[2]],
            "gridSize": grid_size,
            "gridWidth": grid_width,
            "gridDepth": grid_depth,
            "cellSize": cell_size,
            "vertsSpace": "world",
            "verts": verts,
            "tris": tris,
            "heights": heights,
            "heightOrder": "z-major: heights[z * gridWidth + x]",
            "bounds": {
                "min": min_bounds,
                "max": max_bounds,
            },
        }
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

terrain_output_path = os.path.join(
    base_output_path,
    "heighmaps.json"
)

with open(terrain_output_path, "w", encoding="utf-8") as f:
    json.dump({"chunks": terrain_chunks}, f, indent=4)

print(f"Exported {len(terrain_chunks)} terrain chunks to {terrain_output_path}")

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

    # One export per unique type
    if object_type in exported_types:
        continue

    if object_type.startswith("TERRAIN"):
        folder_name = "chunks"
    elif object_type.startswith("ASSET"):
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

    # Select only the current object
    bpy.ops.object.select_all(action="DESELECT")

    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj

    original_location = obj.location.copy()
    original_rotation = obj.rotation_euler.copy()
    original_scale = obj.scale.copy()

    try:
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

        print(
            f"Exported type '{object_type}' "
            f"from object '{obj.name}' to '{output_path}'"
        )

    finally:
        obj.location = original_location
        obj.rotation_euler = original_rotation
        obj.scale = original_scale
        bpy.context.view_layer.update()

print(f"Exported {len(exported_types)} unique object types")

bpy.ops.wm.save_mainfile()