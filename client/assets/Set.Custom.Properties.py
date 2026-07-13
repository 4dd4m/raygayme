import bpy
import uuid

############################################################
#
#    PRE-label all object in scene with a custom property
#
############################################################

created_ids = 0
created_interactions = 0

for obj in bpy.context.scene.objects:

    # ID
    if "id" not in obj or not obj["id"]:
        obj["id"] = str(uuid.uuid4())
        created_ids += 1

    # Interaction
    if "interactive" not in obj:
        obj["interactive"] = False
        created_interactions += 1
        
    if "type" not in obj:
        obj["type"] = ""
        
    if "blenderOnly" not in obj:
        obj["blenderOnly"] = False;
        
    if "chunk" not in obj:
        obj["chunk"] = 0;

print(
    f"Finished.\n"
    f"Created IDs: {created_ids}\n"
    f"Created Interaction properties: {created_interactions}"
)