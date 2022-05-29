bl_info = {
    "name": "myn",
    "blender": (2, 80, 0),
    "category": "Object",
}

import bpy
import bmesh

#---------------------------------------------------------------------

class MYN_OT_origin_to_selected(bpy.types.Operator):
    bl_idname = "myn.origin_to_selected"
    bl_label = "Origin To Selected"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        bpy.ops.view3d.snap_cursor_to_selected()
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='MEDIAN')
        return {'FINISHED'}




class MYN_OT_triangulate_selected(bpy.types.Operator):
    bl_idname = "myn.triangulate_selected"
    bl_label = "Triangulate Selected"
    bl_options = {'REGISTER', 'UNDO'}
    
    @staticmethod
    def triangulate_mesh(data):
        bm = bmesh.new()
        bm.from_mesh(data)
        bmesh.ops.triangulate(bm, faces=bm.faces[:])
        bm.to_mesh(data)
        bm.free()
    
    def execute(self, context):
        bpy.ops.object.mode_set(mode='OBJECT')
        meshes_to_triangulate = set()
        for obj in bpy.context.selected_objects:
            if obj.type == 'MESH':
                meshes_to_triangulate.add(obj.data)
        for m in meshes_to_triangulate:
            #self.report({'INFO'}, m.name)
            MYN_OT_triangulate_selected.triangulate_mesh(m)
            m.update()
            
        self.report({'INFO'}, "trianglated all selected meshes")
        return {'FINISHED'}



class MYN_OT_export_glb(bpy.types.Operator):
    bl_idname = "myn.export_glb"
    bl_label = "Triangulate all and export glb"
    bl_options = {'REGISTER', 'UNDO'}              
    
    def execute(self, context):

        bpy.ops.object.mode_set(mode='OBJECT')
        
        # all mesh data (keep a copy of bmesh original)
        meshes = list()
        bmeshes_original = list()
        
        # add to lists
        for i in range(0, len(bpy.context.scene.objects)):
            obj = bpy.context.scene.objects[i]
            if (obj.type == 'MESH'):
                # meshes
                meshes.append(obj.data)
                # original bmeshes
                bm_o = bmesh.new()
                bm_o.from_mesh(obj.data)
                bmeshes_original.append(bm_o)
                # triangulated bmeshes
                bm_t = bmesh.new()
                bm_t.from_mesh(obj.data)
                bmesh.ops.triangulate(bm_t, faces=bm_t.faces[:])
                bm_t.to_mesh(obj.data)
                bm_t.free()
                obj.data.update()
        
        # export
        # see: https://docs.blender.org/api/current/bpy.ops.export_scene.html
        path=bpy.path.abspath("//" + bpy.context.scene.out_filename)
        bpy.ops.export_scene.gltf(
            filepath=path,
            check_existing=False,
            export_format='GLB',
            export_image_format='AUTO',
            export_texcoords=True,
            export_normals=True,
            export_tangents=True,
            export_materials='EXPORT',
            export_colors=False,
            export_cameras=True,
            export_selected=False,
            use_selection=False,
            use_visible=True,
            use_renderable=False,
            use_active_collection=False,
            export_extras=False,
            export_yup=True,
            export_apply=False,
            export_animations=False,
            export_lights=True
            )
        
        # restore originals
        for i in range(0, len(meshes)):
            bm_o = bmeshes_original[i]
            bm_o.to_mesh(meshes[i])
            bm_o.free()
        
        # report and exit
        self.report({'INFO'}, path)
        
        return {'FINISHED'}


    
#------------------------------ PANEL --------------------------------

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
        
        row = layout.row()
        row.operator("myn.triangulate_selected")
        
        layout.row().separator()
        row = layout.row()
        row.prop(context.scene, "out_filename")
        
        row = layout.row()
        row.operator("myn.export_glb")
        
#------------------------------ REGISTRATION --------------------------------

classes = (
    MYN_OT_origin_to_selected,
    MYN_OT_triangulate_selected,
    MYN_OT_export_glb,
    MYN_PT_tools_panel
    )

def register():
    bpy.types.Scene.out_filename = bpy.props.StringProperty(
        name="output", default="auto_export.glb")
    for cls in classes: bpy.utils.register_class(cls)


def unregister():
    for cls in classes: bpy.utils.unregister_class(cls)


if __name__ == "__main__":
    register()
