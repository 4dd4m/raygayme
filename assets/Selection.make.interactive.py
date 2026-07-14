import bpy

for obj in bpy.context.selected_objects:
    obj["interactive"] = True

print(f"Added 'Interaction=True' to {len(bpy.context.selected_objects)} object(s).")