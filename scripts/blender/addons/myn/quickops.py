import bpy

class MYN_OT_origin_to_selected(bpy.types.Operator):
    bl_idname = "myn.origin_to_selected"
    bl_label = "Origin To Selected"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        bpy.ops.view3d.snap_cursor_to_selected()
        bpy.ops.object.mode_set(mode='OBJECT')
        bpy.ops.object.origin_set(type='ORIGIN_CURSOR', center='MEDIAN')
        return {'FINISHED'}

import os
class MYN_OT_open_scripts_dir(bpy.types.Operator):
    bl_idname = "myn.open_scripts_dir"
    bl_label = "Open Scripts Directory"
    bl_options = {'REGISTER', 'UNDO'}
    def execute(self, context):
        filedir = os.path.dirname(__file__)
        self.report({'INFO'}, filedir)
        os.startfile(filedir)
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
