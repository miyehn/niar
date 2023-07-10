import bpy

class MynProperties(bpy.types.PropertyGroup):

    export_path: bpy.props.StringProperty(
        name="Export Path",
        description="glb export path, relative to this .blend file",
        default="auto_export.glb",
        maxlen=256)
