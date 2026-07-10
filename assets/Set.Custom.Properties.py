import bpy
import uuid

created_ids = 0
created_interactions = 0

for obj in bpy.context.scene.objects:

    # ID
    if "id" not in obj:
        obj["id"] = str(uuid.uuid4())
        created_ids += 1

    # Interaction
    if "Interaction" not in obj:
        obj["Interaction"] = False
        created_interactions += 1

print(
    f"Finished.\n"
    f"Created IDs: {created_ids}\n"
    f"Created Interaction properties: {created_interactions}"
)