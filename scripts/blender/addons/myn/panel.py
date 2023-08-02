import bpy

class MYN_PT_tools_panel(bpy.types.Panel):
    bl_label = "miyehn"
    bl_space_type = "VIEW_3D"
    bl_region_type = "UI"
    bl_category = "miyehn"

    def draw(self, context):
        layout = self.layout

        obj = context.object

        row = layout.row()
        row.label(text="yo sup!", icon='GHOST_ENABLED')
        
        row = layout.row()
        row.operator("myn.origin_to_selected")
        
        #layout.row().separator()
        #row = layout.row()
        #row.operator("myn.triangulate_selected")
        
        layout.row().separator()
        row = layout.row()
        row.prop(context.scene.myn_props, "export_path")
        
        row = layout.row()
        row.operator("myn.export_glb")

        layout.row().separator()
        row = layout.row()
        row.operator("myn.open_scripts_dir")
