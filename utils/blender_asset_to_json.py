import bpy
import json
import os
import uuid

objects = []

#--------------------------------------------------
# Export Every Static Object Position and rotation
#--------------------------------------------------
mapping = ["ASSET_TREE", "ASSET_TREE_TRUNK", "COLLISION_TREE"]

for map in mapping:
    for obj in bpy.context.scene.objects:
        if obj.name.startswith(f"{map}."):
            
            data = obj.name.split("|")
            
            if obj.get("id") is None:
                obj["id"] = str(uuid.uuid4())
                
            if obj.get("interactive") is None:
                raise TypeError(f"interactive property is not set on: {obj.name}")
            
            objects.append({
                "id": obj["id"],
                "interactive": obj.get("interactive"),
                "name": data[0],
                "type": map,
                "chunk": data[1],
                "position": [
                    obj.location.x,
                    obj.location.z,
                    -obj.location.y
                ],
                "rotation": [
                    obj.rotation_euler.x,
                    obj.rotation_euler.z,
                    -obj.rotation_euler.y
                ]
            })

output_path = os.path.join(
    os.path.dirname(bpy.data.filepath),
    "objects.json"
)



with open(output_path, "w", encoding="utf-8") as f:
    if len(objects) == 0:
        f.write("{}")
    else:
        json.dump({"objects": objects}, f, indent=4)

print(f"Exported {len(objects)} objects to {output_path}")

#------------------------------------------------------------------
# Export one of each static asset ["Exact ModelName", AssetId]
#------------------------------------------------------------------
mapping = [
    ["Chunk_0_0","CHUNK_0_0"],
    ["ASSET_TREE.000|0", "ASSET_TREE"],
    ["ASSET_TREE_TRUNK.000|0", "ASSET_TREE_TRUNK"],
    ["COLLISION.001|0", "COLLISION_TREE"],
]


for map in mapping:
    terrain_name = map[0]
    terrain_object = bpy.data.objects.get(terrain_name)

    if terrain_object is None:
        print(f"Terrain object not found: {terrain_name}")
    else:
        
        if "CHUNK" in map[1]:
            pathPrefix = "chunks/"
        elif "ASSET" in map[1]:
            pathPrefix = "static_assets/"
        elif "COLLISION" in map[1]:
            pathPrefix = "collisions/"
        else:
            pathPrefix = ""
        
        bpy.ops.object.select_all(action="DESELECT")

        terrain_object.select_set(True)
        bpy.context.view_layer.objects.active = terrain_object

        terrain_output_path = os.path.join(os.path.dirname(bpy.data.filepath),f"{pathPrefix}{map[1]}.glb")
        
        original_location = terrain_object.location.copy()

        terrain_object.location = (0.0, 0.0, 0.0)

        bpy.ops.export_scene.gltf(
            filepath=terrain_output_path,
            export_format="GLB",
            use_selection=True,
            export_apply=True,
            export_yup=True,
            export_texcoords=True,
            export_normals=True,
            export_materials="EXPORT"
        )
        
        terrain_object.location = original_location