import bpy
import json
import os
import uuid

objects = []

#--------------------------------------------------
# Export Every Static Object Position and rotation
#--------------------------------------------------
mapping = ["ASSET_TREE", "ASSET_TREE_TRUNK", "COLLISION_TREE"]


for obj in bpy.context.scene.objects:
    
    if obj.get("blenderOnly") is None:
        raise TypeError("f{obj.name} has no blenderOnly property")
        
    if obj.get("blenderOnly") == True or obj.name == "Chunk_0_0":
        continue

  
    data = obj.name.split("|")
    
    if obj.get("id") is None:
        obj["id"] = str(uuid.uuid4())
        
    if obj.get("interactive") is None:
        raise TypeError(f"interactive property is not set on: {obj.name}")
        
    if obj.get("type") is None:
        raise TypeError(f"{ob.name} has no type")
        
    if obj.get("chunk") is None:
        raise TypeError(f"{obj.name} has no chunkId")
    
    objects.append({
        "id": obj.get("id"),
        "interactive": obj.get("interactive"),
        "name": obj.name,
        "type":  obj.get("type"),
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
# Export one of each object
#------------------------------------------------------------------

exported_types = set()

base_output_path = os.path.dirname(bpy.data.filepath)

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

    try:
        # Export the model around the world origin
        obj.location = (0.0, 0.0, 0.0)

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
        bpy.context.view_layer.update()

print(f"Exported {len(exported_types)} unique object types")