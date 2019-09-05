#include "GL.hpp"

#include <SDL.h>
#include <iostream>
#include <stdexcept>

#ifdef _WIN32
	#define DO(fn) \
		fn = (decltype(fn))SDL_GL_GetProcAddress(#fn); \
		if (!fn) { \
			throw std::runtime_error("Error binding " #fn); \
		}
#else
	#define DO(fn)
#endif

void init_GL() {
	DO(glDrawRangeElements)
	DO(glTexImage3D)
	DO(glTexSubImage3D)
	DO(glCopyTexSubImage3D)
	DO(glActiveTexture)
	DO(glSampleCoverage)
	DO(glCompressedTexImage3D)
	DO(glCompressedTexImage2D)
	DO(glCompressedTexImage1D)
	DO(glCompressedTexSubImage3D)
	DO(glCompressedTexSubImage2D)
	DO(glCompressedTexSubImage1D)
	DO(glGetCompressedTexImage)
	DO(glBlendFuncSeparate)
	DO(glMultiDrawArrays)
	DO(glMultiDrawElements)
	DO(glPointParameterf)
	DO(glPointParameterfv)
	DO(glPointParameteri)
	DO(glPointParameteriv)
	DO(glBlendColor)
	DO(glBlendEquation)
	DO(glGenQueries)
	DO(glDeleteQueries)
	DO(glIsQuery)
	DO(glBeginQuery)
	DO(glEndQuery)
	DO(glGetQueryiv)
	DO(glGetQueryObjectiv)
	DO(glGetQueryObjectuiv)
	DO(glBindBuffer)
	DO(glDeleteBuffers)
	DO(glGenBuffers)
	DO(glIsBuffer)
	DO(glBufferData)
	DO(glBufferSubData)
	DO(glGetBufferSubData)
	DO(glMapBuffer)
	DO(glUnmapBuffer)
	DO(glGetBufferParameteriv)
	DO(glGetBufferPointerv)
	DO(glBlendEquationSeparate)
	DO(glDrawBuffers)
	DO(glStencilOpSeparate)
	DO(glStencilFuncSeparate)
	DO(glStencilMaskSeparate)
	DO(glAttachShader)
	DO(glBindAttribLocation)
	DO(glCompileShader)
	DO(glCreateProgram)
	DO(glCreateShader)
	DO(glDeleteProgram)
	DO(glDeleteShader)
	DO(glDetachShader)
	DO(glDisableVertexAttribArray)
	DO(glEnableVertexAttribArray)
	DO(glGetActiveAttrib)
	DO(glGetActiveUniform)
	DO(glGetAttachedShaders)
	DO(glGetAttribLocation)
	DO(glGetProgramiv)
	DO(glGetProgramInfoLog)
	DO(glGetShaderiv)
	DO(glGetShaderInfoLog)
	DO(glGetShaderSource)
	DO(glGetUniformLocation)
	DO(glGetUniformfv)
	DO(glGetUniformiv)
	DO(glGetVertexAttribdv)
	DO(glGetVertexAttribfv)
	DO(glGetVertexAttribiv)
	DO(glGetVertexAttribPointerv)
	DO(glIsProgram)
	DO(glIsShader)
	DO(glLinkProgram)
	DO(glShaderSource)
	DO(glUseProgram)
	DO(glUniform1f)
	DO(glUniform2f)
	DO(glUniform3f)
	DO(glUniform4f)
	DO(glUniform1i)
	DO(glUniform2i)
	DO(glUniform3i)
	DO(glUniform4i)
	DO(glUniform1fv)
	DO(glUniform2fv)
	DO(glUniform3fv)
	DO(glUniform4fv)
	DO(glUniform1iv)
	DO(glUniform2iv)
	DO(glUniform3iv)
	DO(glUniform4iv)
	DO(glUniformMatrix2fv)
	DO(glUniformMatrix3fv)
	DO(glUniformMatrix4fv)
	DO(glValidateProgram)
	DO(glVertexAttrib1d)
	DO(glVertexAttrib1dv)
	DO(glVertexAttrib1f)
	DO(glVertexAttrib1fv)
	DO(glVertexAttrib1s)
	DO(glVertexAttrib1sv)
	DO(glVertexAttrib2d)
	DO(glVertexAttrib2dv)
	DO(glVertexAttrib2f)
	DO(glVertexAttrib2fv)
	DO(glVertexAttrib2s)
	DO(glVertexAttrib2sv)
	DO(glVertexAttrib3d)
	DO(glVertexAttrib3dv)
	DO(glVertexAttrib3f)
	DO(glVertexAttrib3fv)
	DO(glVertexAttrib3s)
	DO(glVertexAttrib3sv)
	DO(glVertexAttrib4Nbv)
	DO(glVertexAttrib4Niv)
	DO(glVertexAttrib4Nsv)
	DO(glVertexAttrib4Nub)
	DO(glVertexAttrib4Nubv)
	DO(glVertexAttrib4Nuiv)
	DO(glVertexAttrib4Nusv)
	DO(glVertexAttrib4bv)
	DO(glVertexAttrib4d)
	DO(glVertexAttrib4dv)
	DO(glVertexAttrib4f)
	DO(glVertexAttrib4fv)
	DO(glVertexAttrib4iv)
	DO(glVertexAttrib4s)
	DO(glVertexAttrib4sv)
	DO(glVertexAttrib4ubv)
	DO(glVertexAttrib4uiv)
	DO(glVertexAttrib4usv)
	DO(glVertexAttribPointer)
	DO(glUniformMatrix2x3fv)
	DO(glUniformMatrix3x2fv)
	DO(glUniformMatrix2x4fv)
	DO(glUniformMatrix4x2fv)
	DO(glUniformMatrix3x4fv)
	DO(glUniformMatrix4x3fv)
	DO(glColorMaski)
	DO(glGetBooleani_v)
	DO(glGetIntegeri_v)
	DO(glEnablei)
	DO(glDisablei)
	DO(glIsEnabledi)
	DO(glBeginTransformFeedback)
	DO(glEndTransformFeedback)
	DO(glBindBufferRange)
	DO(glBindBufferBase)
	DO(glTransformFeedbackVaryings)
	DO(glGetTransformFeedbackVarying)
	DO(glClampColor)
	DO(glBeginConditionalRender)
	DO(glEndConditionalRender)
	DO(glVertexAttribIPointer)
	DO(glGetVertexAttribIiv)
	DO(glGetVertexAttribIuiv)
	DO(glVertexAttribI1i)
	DO(glVertexAttribI2i)
	DO(glVertexAttribI3i)
	DO(glVertexAttribI4i)
	DO(glVertexAttribI1ui)
	DO(glVertexAttribI2ui)
	DO(glVertexAttribI3ui)
	DO(glVertexAttribI4ui)
	DO(glVertexAttribI1iv)
	DO(glVertexAttribI2iv)
	DO(glVertexAttribI3iv)
	DO(glVertexAttribI4iv)
	DO(glVertexAttribI1uiv)
	DO(glVertexAttribI2uiv)
	DO(glVertexAttribI3uiv)
	DO(glVertexAttribI4uiv)
	DO(glVertexAttribI4bv)
	DO(glVertexAttribI4sv)
	DO(glVertexAttribI4ubv)
	DO(glVertexAttribI4usv)
	DO(glGetUniformuiv)
	DO(glBindFragDataLocation)
	DO(glGetFragDataLocation)
	DO(glUniform1ui)
	DO(glUniform2ui)
	DO(glUniform3ui)
	DO(glUniform4ui)
	DO(glUniform1uiv)
	DO(glUniform2uiv)
	DO(glUniform3uiv)
	DO(glUniform4uiv)
	DO(glTexParameterIiv)
	DO(glTexParameterIuiv)
	DO(glGetTexParameterIiv)
	DO(glGetTexParameterIuiv)
	DO(glClearBufferiv)
	DO(glClearBufferuiv)
	DO(glClearBufferfv)
	DO(glClearBufferfi)
	DO(glGetStringi)
	DO(glIsRenderbuffer)
	DO(glBindRenderbuffer)
	DO(glDeleteRenderbuffers)
	DO(glGenRenderbuffers)
	DO(glRenderbufferStorage)
	DO(glGetRenderbufferParameteriv)
	DO(glIsFramebuffer)
	DO(glBindFramebuffer)
	DO(glDeleteFramebuffers)
	DO(glGenFramebuffers)
	DO(glCheckFramebufferStatus)
	DO(glFramebufferTexture1D)
	DO(glFramebufferTexture2D)
	DO(glFramebufferTexture3D)
	DO(glFramebufferRenderbuffer)
	DO(glGetFramebufferAttachmentParameteriv)
	DO(glGenerateMipmap)
	DO(glBlitFramebuffer)
	DO(glRenderbufferStorageMultisample)
	DO(glFramebufferTextureLayer)
	DO(glMapBufferRange)
	DO(glFlushMappedBufferRange)
	DO(glBindVertexArray)
	DO(glDeleteVertexArrays)
	DO(glGenVertexArrays)
	DO(glIsVertexArray)
	DO(glDrawArraysInstanced)
	DO(glDrawElementsInstanced)
	DO(glTexBuffer)
	DO(glPrimitiveRestartIndex)
	DO(glCopyBufferSubData)
	DO(glGetUniformIndices)
	DO(glGetActiveUniformsiv)
	DO(glGetActiveUniformName)
	DO(glGetUniformBlockIndex)
	DO(glGetActiveUniformBlockiv)
	DO(glGetActiveUniformBlockName)
	DO(glUniformBlockBinding)
	DO(glDrawElementsBaseVertex)
	DO(glDrawRangeElementsBaseVertex)
	DO(glDrawElementsInstancedBaseVertex)
	DO(glMultiDrawElementsBaseVertex)
	DO(glProvokingVertex)
	DO(glFenceSync)
	DO(glIsSync)
	DO(glDeleteSync)
	DO(glClientWaitSync)
	DO(glWaitSync)
	DO(glGetInteger64v)
	DO(glGetSynciv)
	DO(glGetInteger64i_v)
	DO(glGetBufferParameteri64v)
	DO(glFramebufferTexture)
	DO(glTexImage2DMultisample)
	DO(glTexImage3DMultisample)
	DO(glGetMultisamplefv)
	DO(glSampleMaski)
	DO(glBindFragDataLocationIndexed)
	DO(glGetFragDataIndex)
	DO(glGenSamplers)
	DO(glDeleteSamplers)
	DO(glIsSampler)
	DO(glBindSampler)
	DO(glSamplerParameteri)
	DO(glSamplerParameteriv)
	DO(glSamplerParameterf)
	DO(glSamplerParameterfv)
	DO(glSamplerParameterIiv)
	DO(glSamplerParameterIuiv)
	DO(glGetSamplerParameteriv)
	DO(glGetSamplerParameterIiv)
	DO(glGetSamplerParameterfv)
	DO(glGetSamplerParameterIuiv)
	DO(glQueryCounter)
	DO(glGetQueryObjecti64v)
	DO(glGetQueryObjectui64v)
	DO(glVertexAttribDivisor)
	DO(glVertexAttribP1ui)
	DO(glVertexAttribP1uiv)
	DO(glVertexAttribP2ui)
	DO(glVertexAttribP2uiv)
	DO(glVertexAttribP3ui)
	DO(glVertexAttribP3uiv)
	DO(glVertexAttribP4ui)
	DO(glVertexAttribP4uiv)
	DO(glMinSampleShading)
	DO(glBlendEquationi)
	DO(glBlendEquationSeparatei)
	DO(glBlendFunci)
	DO(glBlendFuncSeparatei)
	DO(glDrawArraysIndirect)
	DO(glDrawElementsIndirect)
	DO(glUniform1d)
	DO(glUniform2d)
	DO(glUniform3d)
	DO(glUniform4d)
	DO(glUniform1dv)
	DO(glUniform2dv)
	DO(glUniform3dv)
	DO(glUniform4dv)
	DO(glUniformMatrix2dv)
	DO(glUniformMatrix3dv)
	DO(glUniformMatrix4dv)
	DO(glUniformMatrix2x3dv)
	DO(glUniformMatrix2x4dv)
	DO(glUniformMatrix3x2dv)
	DO(glUniformMatrix3x4dv)
	DO(glUniformMatrix4x2dv)
	DO(glUniformMatrix4x3dv)
	DO(glGetUniformdv)
	DO(glGetSubroutineUniformLocation)
	DO(glGetSubroutineIndex)
	DO(glGetActiveSubroutineUniformiv)
	DO(glGetActiveSubroutineUniformName)
	DO(glGetActiveSubroutineName)
	DO(glUniformSubroutinesuiv)
	DO(glGetUniformSubroutineuiv)
	DO(glGetProgramStageiv)
	DO(glPatchParameteri)
	DO(glPatchParameterfv)
	DO(glBindTransformFeedback)
	DO(glDeleteTransformFeedbacks)
	DO(glGenTransformFeedbacks)
	DO(glIsTransformFeedback)
	DO(glPauseTransformFeedback)
	DO(glResumeTransformFeedback)
	DO(glDrawTransformFeedback)
	DO(glDrawTransformFeedbackStream)
	DO(glBeginQueryIndexed)
	DO(glEndQueryIndexed)
	DO(glGetQueryIndexediv)
	DO(glReleaseShaderCompiler)
	DO(glShaderBinary)
	DO(glGetShaderPrecisionFormat)
	DO(glDepthRangef)
	DO(glClearDepthf)
	DO(glGetProgramBinary)
	DO(glProgramBinary)
	DO(glProgramParameteri)
	DO(glUseProgramStages)
	DO(glActiveShaderProgram)
	DO(glCreateShaderProgramv)
	DO(glBindProgramPipeline)
	DO(glDeleteProgramPipelines)
	DO(glGenProgramPipelines)
	DO(glIsProgramPipeline)
	DO(glGetProgramPipelineiv)
	DO(glProgramUniform1i)
	DO(glProgramUniform1iv)
	DO(glProgramUniform1f)
	DO(glProgramUniform1fv)
	DO(glProgramUniform1d)
	DO(glProgramUniform1dv)
	DO(glProgramUniform1ui)
	DO(glProgramUniform1uiv)
	DO(glProgramUniform2i)
	DO(glProgramUniform2iv)
	DO(glProgramUniform2f)
	DO(glProgramUniform2fv)
	DO(glProgramUniform2d)
	DO(glProgramUniform2dv)
	DO(glProgramUniform2ui)
	DO(glProgramUniform2uiv)
	DO(glProgramUniform3i)
	DO(glProgramUniform3iv)
	DO(glProgramUniform3f)
	DO(glProgramUniform3fv)
	DO(glProgramUniform3d)
	DO(glProgramUniform3dv)
	DO(glProgramUniform3ui)
	DO(glProgramUniform3uiv)
	DO(glProgramUniform4i)
	DO(glProgramUniform4iv)
	DO(glProgramUniform4f)
	DO(glProgramUniform4fv)
	DO(glProgramUniform4d)
	DO(glProgramUniform4dv)
	DO(glProgramUniform4ui)
	DO(glProgramUniform4uiv)
	DO(glProgramUniformMatrix2fv)
	DO(glProgramUniformMatrix3fv)
	DO(glProgramUniformMatrix4fv)
	DO(glProgramUniformMatrix2dv)
	DO(glProgramUniformMatrix3dv)
	DO(glProgramUniformMatrix4dv)
	DO(glProgramUniformMatrix2x3fv)
	DO(glProgramUniformMatrix3x2fv)
	DO(glProgramUniformMatrix2x4fv)
	DO(glProgramUniformMatrix4x2fv)
	DO(glProgramUniformMatrix3x4fv)
	DO(glProgramUniformMatrix4x3fv)
	DO(glProgramUniformMatrix2x3dv)
	DO(glProgramUniformMatrix3x2dv)
	DO(glProgramUniformMatrix2x4dv)
	DO(glProgramUniformMatrix4x2dv)
	DO(glProgramUniformMatrix3x4dv)
	DO(glProgramUniformMatrix4x3dv)
	DO(glValidateProgramPipeline)
	DO(glGetProgramPipelineInfoLog)
	DO(glVertexAttribL1d)
	DO(glVertexAttribL2d)
	DO(glVertexAttribL3d)
	DO(glVertexAttribL4d)
	DO(glVertexAttribL1dv)
	DO(glVertexAttribL2dv)
	DO(glVertexAttribL3dv)
	DO(glVertexAttribL4dv)
	DO(glVertexAttribLPointer)
	DO(glGetVertexAttribLdv)
	DO(glViewportArrayv)
	DO(glViewportIndexedf)
	DO(glViewportIndexedfv)
	DO(glScissorArrayv)
	DO(glScissorIndexed)
	DO(glScissorIndexedv)
	DO(glDepthRangeArrayv)
	DO(glDepthRangeIndexed)
	DO(glGetFloati_v)
	DO(glGetDoublei_v)
	DO(glDrawArraysInstancedBaseInstance)
	DO(glDrawElementsInstancedBaseInstance)
	DO(glDrawElementsInstancedBaseVertexBaseInstance)
	DO(glGetInternalformativ)
	DO(glGetActiveAtomicCounterBufferiv)
	DO(glBindImageTexture)
	DO(glMemoryBarrier)
	DO(glTexStorage1D)
	DO(glTexStorage2D)
	DO(glTexStorage3D)
	DO(glDrawTransformFeedbackInstanced)
	DO(glDrawTransformFeedbackStreamInstanced)
	DO(glClearBufferData)
	DO(glClearBufferSubData)
	DO(glDispatchCompute)
	DO(glDispatchComputeIndirect)
	DO(glCopyImageSubData)
	DO(glFramebufferParameteri)
	DO(glGetFramebufferParameteriv)
	DO(glGetInternalformati64v)
	DO(glInvalidateTexSubImage)
	DO(glInvalidateTexImage)
	DO(glInvalidateBufferSubData)
	DO(glInvalidateBufferData)
	DO(glInvalidateFramebuffer)
	DO(glInvalidateSubFramebuffer)
	DO(glMultiDrawArraysIndirect)
	DO(glMultiDrawElementsIndirect)
	DO(glGetProgramInterfaceiv)
	DO(glGetProgramResourceIndex)
	DO(glGetProgramResourceName)
	DO(glGetProgramResourceiv)
	DO(glGetProgramResourceLocation)
	DO(glGetProgramResourceLocationIndex)
	DO(glShaderStorageBlockBinding)
	DO(glTexBufferRange)
	DO(glTexStorage2DMultisample)
	DO(glTexStorage3DMultisample)
	DO(glTextureView)
	DO(glBindVertexBuffer)
	DO(glVertexAttribFormat)
	DO(glVertexAttribIFormat)
	DO(glVertexAttribLFormat)
	DO(glVertexAttribBinding)
	DO(glVertexBindingDivisor)
	DO(glDebugMessageControl)
	DO(glDebugMessageInsert)
	DO(glDebugMessageCallback)
	DO(glGetDebugMessageLog)
	DO(glPushDebugGroup)
	DO(glPopDebugGroup)
	DO(glObjectLabel)
	DO(glGetObjectLabel)
	DO(glObjectPtrLabel)
	DO(glGetObjectPtrLabel)
	DO(glBufferStorage)
	DO(glClearTexImage)
	DO(glClearTexSubImage)
	DO(glBindBuffersBase)
	DO(glBindBuffersRange)
	DO(glBindTextures)
	DO(glBindSamplers)
	DO(glBindImageTextures)
	DO(glBindVertexBuffers)
	DO(glClipControl)
	DO(glCreateTransformFeedbacks)
	DO(glTransformFeedbackBufferBase)
	DO(glTransformFeedbackBufferRange)
	DO(glGetTransformFeedbackiv)
	DO(glGetTransformFeedbacki_v)
	DO(glGetTransformFeedbacki64_v)
	DO(glCreateBuffers)
	DO(glNamedBufferStorage)
	DO(glNamedBufferData)
	DO(glNamedBufferSubData)
	DO(glCopyNamedBufferSubData)
	DO(glClearNamedBufferData)
	DO(glClearNamedBufferSubData)
	DO(glMapNamedBuffer)
	DO(glMapNamedBufferRange)
	DO(glUnmapNamedBuffer)
	DO(glFlushMappedNamedBufferRange)
	DO(glGetNamedBufferParameteriv)
	DO(glGetNamedBufferParameteri64v)
	DO(glGetNamedBufferPointerv)
	DO(glGetNamedBufferSubData)
	DO(glCreateFramebuffers)
	DO(glNamedFramebufferRenderbuffer)
	DO(glNamedFramebufferParameteri)
	DO(glNamedFramebufferTexture)
	DO(glNamedFramebufferTextureLayer)
	DO(glNamedFramebufferDrawBuffer)
	DO(glNamedFramebufferDrawBuffers)
	DO(glNamedFramebufferReadBuffer)
	DO(glInvalidateNamedFramebufferData)
	DO(glInvalidateNamedFramebufferSubData)
	DO(glClearNamedFramebufferiv)
	DO(glClearNamedFramebufferuiv)
	DO(glClearNamedFramebufferfv)
	DO(glClearNamedFramebufferfi)
	DO(glBlitNamedFramebuffer)
	DO(glCheckNamedFramebufferStatus)
	DO(glGetNamedFramebufferParameteriv)
	DO(glGetNamedFramebufferAttachmentParameteriv)
	DO(glCreateRenderbuffers)
	DO(glNamedRenderbufferStorage)
	DO(glNamedRenderbufferStorageMultisample)
	DO(glGetNamedRenderbufferParameteriv)
	DO(glCreateTextures)
	DO(glTextureBuffer)
	DO(glTextureBufferRange)
	DO(glTextureStorage1D)
	DO(glTextureStorage2D)
	DO(glTextureStorage3D)
	DO(glTextureStorage2DMultisample)
	DO(glTextureStorage3DMultisample)
	DO(glTextureSubImage1D)
	DO(glTextureSubImage2D)
	DO(glTextureSubImage3D)
	DO(glCompressedTextureSubImage1D)
	DO(glCompressedTextureSubImage2D)
	DO(glCompressedTextureSubImage3D)
	DO(glCopyTextureSubImage1D)
	DO(glCopyTextureSubImage2D)
	DO(glCopyTextureSubImage3D)
	DO(glTextureParameterf)
	DO(glTextureParameterfv)
	DO(glTextureParameteri)
	DO(glTextureParameterIiv)
	DO(glTextureParameterIuiv)
	DO(glTextureParameteriv)
	DO(glGenerateTextureMipmap)
	DO(glBindTextureUnit)
	DO(glGetTextureImage)
	DO(glGetCompressedTextureImage)
	DO(glGetTextureLevelParameterfv)
	DO(glGetTextureLevelParameteriv)
	DO(glGetTextureParameterfv)
	DO(glGetTextureParameterIiv)
	DO(glGetTextureParameterIuiv)
	DO(glGetTextureParameteriv)
	DO(glCreateVertexArrays)
	DO(glDisableVertexArrayAttrib)
	DO(glEnableVertexArrayAttrib)
	DO(glVertexArrayElementBuffer)
	DO(glVertexArrayVertexBuffer)
	DO(glVertexArrayVertexBuffers)
	DO(glVertexArrayAttribBinding)
	DO(glVertexArrayAttribFormat)
	DO(glVertexArrayAttribIFormat)
	DO(glVertexArrayAttribLFormat)
	DO(glVertexArrayBindingDivisor)
	DO(glGetVertexArrayiv)
	DO(glGetVertexArrayIndexediv)
	DO(glGetVertexArrayIndexed64iv)
	DO(glCreateSamplers)
	DO(glCreateProgramPipelines)
	DO(glCreateQueries)
	DO(glGetQueryBufferObjecti64v)
	DO(glGetQueryBufferObjectiv)
	DO(glGetQueryBufferObjectui64v)
	DO(glGetQueryBufferObjectuiv)
	DO(glMemoryBarrierByRegion)
	DO(glGetTextureSubImage)
	DO(glGetCompressedTextureSubImage)
	DO(glGetGraphicsResetStatus)
	DO(glGetnCompressedTexImage)
	DO(glGetnTexImage)
	DO(glGetnUniformdv)
	DO(glGetnUniformfv)
	DO(glGetnUniformiv)
	DO(glGetnUniformuiv)
	DO(glReadnPixels)
	DO(glTextureBarrier)
}
#ifdef _WIN32
	 void (APIENTRYFP glDrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
	 void (APIENTRYFP glTexImage3D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const void *pixels);
	 void (APIENTRYFP glTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	 void (APIENTRYFP glCopyTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	 void (APIENTRYFP glActiveTexture) (GLenum texture);
	 void (APIENTRYFP glSampleCoverage) (GLfloat value, GLboolean invert);
	 void (APIENTRYFP glCompressedTexImage3D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTexImage1D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glGetCompressedTexImage) (GLenum target, GLint level, void *img);
	 void (APIENTRYFP glBlendFuncSeparate) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
	 void (APIENTRYFP glMultiDrawArrays) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei drawcount);
	 void (APIENTRYFP glMultiDrawElements) (GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount);
	 void (APIENTRYFP glPointParameterf) (GLenum pname, GLfloat param);
	 void (APIENTRYFP glPointParameterfv) (GLenum pname, const GLfloat *params);
	 void (APIENTRYFP glPointParameteri) (GLenum pname, GLint param);
	 void (APIENTRYFP glPointParameteriv) (GLenum pname, const GLint *params);
	 void (APIENTRYFP glBlendColor) (GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha);
	 void (APIENTRYFP glBlendEquation) (GLenum mode);
	 void (APIENTRYFP glGenQueries) (GLsizei n, GLuint *ids);
	 void (APIENTRYFP glDeleteQueries) (GLsizei n, const GLuint *ids);
	 GLboolean (APIENTRYFP glIsQuery) (GLuint id);
	 void (APIENTRYFP glBeginQuery) (GLenum target, GLuint id);
	 void (APIENTRYFP glEndQuery) (GLenum target);
	 void (APIENTRYFP glGetQueryiv) (GLenum target, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetQueryObjectiv) (GLuint id, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetQueryObjectuiv) (GLuint id, GLenum pname, GLuint *params);
	 void (APIENTRYFP glBindBuffer) (GLenum target, GLuint buffer);
	 void (APIENTRYFP glDeleteBuffers) (GLsizei n, const GLuint *buffers);
	 void (APIENTRYFP glGenBuffers) (GLsizei n, GLuint *buffers);
	 GLboolean (APIENTRYFP glIsBuffer) (GLuint buffer);
	 void (APIENTRYFP glBufferData) (GLenum target, GLsizeiptr size, const void *data, GLenum usage);
	 void (APIENTRYFP glBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const void *data);
	 void (APIENTRYFP glGetBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, void *data);
	 void *(APIENTRYFP glMapBuffer) (GLenum target, GLenum access);
	 GLboolean (APIENTRYFP glUnmapBuffer) (GLenum target);
	 void (APIENTRYFP glGetBufferParameteriv) (GLenum target, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetBufferPointerv) (GLenum target, GLenum pname, void **params);
	 void (APIENTRYFP glBlendEquationSeparate) (GLenum modeRGB, GLenum modeAlpha);
	 void (APIENTRYFP glDrawBuffers) (GLsizei n, const GLenum *bufs);
	 void (APIENTRYFP glStencilOpSeparate) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
	 void (APIENTRYFP glStencilFuncSeparate) (GLenum face, GLenum func, GLint ref, GLuint mask);
	 void (APIENTRYFP glStencilMaskSeparate) (GLenum face, GLuint mask);
	 void (APIENTRYFP glAttachShader) (GLuint program, GLuint shader);
	 void (APIENTRYFP glBindAttribLocation) (GLuint program, GLuint index, const GLchar *name);
	 void (APIENTRYFP glCompileShader) (GLuint shader);
	 GLuint (APIENTRYFP glCreateProgram) (void);
	 GLuint (APIENTRYFP glCreateShader) (GLenum type);
	 void (APIENTRYFP glDeleteProgram) (GLuint program);
	 void (APIENTRYFP glDeleteShader) (GLuint shader);
	 void (APIENTRYFP glDetachShader) (GLuint program, GLuint shader);
	 void (APIENTRYFP glDisableVertexAttribArray) (GLuint index);
	 void (APIENTRYFP glEnableVertexAttribArray) (GLuint index);
	 void (APIENTRYFP glGetActiveAttrib) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
	 void (APIENTRYFP glGetActiveUniform) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLint *size, GLenum *type, GLchar *name);
	 void (APIENTRYFP glGetAttachedShaders) (GLuint program, GLsizei maxCount, GLsizei *count, GLuint *shaders);
	 GLint (APIENTRYFP glGetAttribLocation) (GLuint program, const GLchar *name);
	 void (APIENTRYFP glGetProgramiv) (GLuint program, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetProgramInfoLog) (GLuint program, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	 void (APIENTRYFP glGetShaderiv) (GLuint shader, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetShaderInfoLog) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	 void (APIENTRYFP glGetShaderSource) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *source);
	 GLint (APIENTRYFP glGetUniformLocation) (GLuint program, const GLchar *name);
	 void (APIENTRYFP glGetUniformfv) (GLuint program, GLint location, GLfloat *params);
	 void (APIENTRYFP glGetUniformiv) (GLuint program, GLint location, GLint *params);
	 void (APIENTRYFP glGetVertexAttribdv) (GLuint index, GLenum pname, GLdouble *params);
	 void (APIENTRYFP glGetVertexAttribfv) (GLuint index, GLenum pname, GLfloat *params);
	 void (APIENTRYFP glGetVertexAttribiv) (GLuint index, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetVertexAttribPointerv) (GLuint index, GLenum pname, void **pointer);
	 GLboolean (APIENTRYFP glIsProgram) (GLuint program);
	 GLboolean (APIENTRYFP glIsShader) (GLuint shader);
	 void (APIENTRYFP glLinkProgram) (GLuint program);
	 void (APIENTRYFP glShaderSource) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
	 void (APIENTRYFP glUseProgram) (GLuint program);
	 void (APIENTRYFP glUniform1f) (GLint location, GLfloat v0);
	 void (APIENTRYFP glUniform2f) (GLint location, GLfloat v0, GLfloat v1);
	 void (APIENTRYFP glUniform3f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	 void (APIENTRYFP glUniform4f) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	 void (APIENTRYFP glUniform1i) (GLint location, GLint v0);
	 void (APIENTRYFP glUniform2i) (GLint location, GLint v0, GLint v1);
	 void (APIENTRYFP glUniform3i) (GLint location, GLint v0, GLint v1, GLint v2);
	 void (APIENTRYFP glUniform4i) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	 void (APIENTRYFP glUniform1fv) (GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glUniform2fv) (GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glUniform3fv) (GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glUniform4fv) (GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glUniform1iv) (GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glUniform2iv) (GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glUniform3iv) (GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glUniform4iv) (GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glUniformMatrix2fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glValidateProgram) (GLuint program);
	 void (APIENTRYFP glVertexAttrib1d) (GLuint index, GLdouble x);
	 void (APIENTRYFP glVertexAttrib1dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttrib1f) (GLuint index, GLfloat x);
	 void (APIENTRYFP glVertexAttrib1fv) (GLuint index, const GLfloat *v);
	 void (APIENTRYFP glVertexAttrib1s) (GLuint index, GLshort x);
	 void (APIENTRYFP glVertexAttrib1sv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttrib2d) (GLuint index, GLdouble x, GLdouble y);
	 void (APIENTRYFP glVertexAttrib2dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttrib2f) (GLuint index, GLfloat x, GLfloat y);
	 void (APIENTRYFP glVertexAttrib2fv) (GLuint index, const GLfloat *v);
	 void (APIENTRYFP glVertexAttrib2s) (GLuint index, GLshort x, GLshort y);
	 void (APIENTRYFP glVertexAttrib2sv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttrib3d) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
	 void (APIENTRYFP glVertexAttrib3dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttrib3f) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
	 void (APIENTRYFP glVertexAttrib3fv) (GLuint index, const GLfloat *v);
	 void (APIENTRYFP glVertexAttrib3s) (GLuint index, GLshort x, GLshort y, GLshort z);
	 void (APIENTRYFP glVertexAttrib3sv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttrib4Nbv) (GLuint index, const GLbyte *v);
	 void (APIENTRYFP glVertexAttrib4Niv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttrib4Nsv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttrib4Nub) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
	 void (APIENTRYFP glVertexAttrib4Nubv) (GLuint index, const GLubyte *v);
	 void (APIENTRYFP glVertexAttrib4Nuiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttrib4Nusv) (GLuint index, const GLushort *v);
	 void (APIENTRYFP glVertexAttrib4bv) (GLuint index, const GLbyte *v);
	 void (APIENTRYFP glVertexAttrib4d) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	 void (APIENTRYFP glVertexAttrib4dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttrib4f) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
	 void (APIENTRYFP glVertexAttrib4fv) (GLuint index, const GLfloat *v);
	 void (APIENTRYFP glVertexAttrib4iv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttrib4s) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
	 void (APIENTRYFP glVertexAttrib4sv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttrib4ubv) (GLuint index, const GLubyte *v);
	 void (APIENTRYFP glVertexAttrib4uiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttrib4usv) (GLuint index, const GLushort *v);
	 void (APIENTRYFP glVertexAttribPointer) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const void *pointer);
	 void (APIENTRYFP glUniformMatrix2x3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix3x2fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix2x4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix4x2fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix3x4fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glUniformMatrix4x3fv) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glColorMaski) (GLuint index, GLboolean r, GLboolean g, GLboolean b, GLboolean a);
	 void (APIENTRYFP glGetBooleani_v) (GLenum target, GLuint index, GLboolean *data);
	 void (APIENTRYFP glGetIntegeri_v) (GLenum target, GLuint index, GLint *data);
	 void (APIENTRYFP glEnablei) (GLenum target, GLuint index);
	 void (APIENTRYFP glDisablei) (GLenum target, GLuint index);
	 GLboolean (APIENTRYFP glIsEnabledi) (GLenum target, GLuint index);
	 void (APIENTRYFP glBeginTransformFeedback) (GLenum primitiveMode);
	 void (APIENTRYFP glEndTransformFeedback) (void);
	 void (APIENTRYFP glBindBufferRange) (GLenum target, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	 void (APIENTRYFP glBindBufferBase) (GLenum target, GLuint index, GLuint buffer);
	 void (APIENTRYFP glTransformFeedbackVaryings) (GLuint program, GLsizei count, const GLchar *const*varyings, GLenum bufferMode);
	 void (APIENTRYFP glGetTransformFeedbackVarying) (GLuint program, GLuint index, GLsizei bufSize, GLsizei *length, GLsizei *size, GLenum *type, GLchar *name);
	 void (APIENTRYFP glClampColor) (GLenum target, GLenum clamp);
	 void (APIENTRYFP glBeginConditionalRender) (GLuint id, GLenum mode);
	 void (APIENTRYFP glEndConditionalRender) (void);
	 void (APIENTRYFP glVertexAttribIPointer) (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	 void (APIENTRYFP glGetVertexAttribIiv) (GLuint index, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetVertexAttribIuiv) (GLuint index, GLenum pname, GLuint *params);
	 void (APIENTRYFP glVertexAttribI1i) (GLuint index, GLint x);
	 void (APIENTRYFP glVertexAttribI2i) (GLuint index, GLint x, GLint y);
	 void (APIENTRYFP glVertexAttribI3i) (GLuint index, GLint x, GLint y, GLint z);
	 void (APIENTRYFP glVertexAttribI4i) (GLuint index, GLint x, GLint y, GLint z, GLint w);
	 void (APIENTRYFP glVertexAttribI1ui) (GLuint index, GLuint x);
	 void (APIENTRYFP glVertexAttribI2ui) (GLuint index, GLuint x, GLuint y);
	 void (APIENTRYFP glVertexAttribI3ui) (GLuint index, GLuint x, GLuint y, GLuint z);
	 void (APIENTRYFP glVertexAttribI4ui) (GLuint index, GLuint x, GLuint y, GLuint z, GLuint w);
	 void (APIENTRYFP glVertexAttribI1iv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttribI2iv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttribI3iv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttribI4iv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glVertexAttribI1uiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttribI2uiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttribI3uiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttribI4uiv) (GLuint index, const GLuint *v);
	 void (APIENTRYFP glVertexAttribI4bv) (GLuint index, const GLbyte *v);
	 void (APIENTRYFP glVertexAttribI4sv) (GLuint index, const GLshort *v);
	 void (APIENTRYFP glVertexAttribI4ubv) (GLuint index, const GLubyte *v);
	 void (APIENTRYFP glVertexAttribI4usv) (GLuint index, const GLushort *v);
	 void (APIENTRYFP glGetUniformuiv) (GLuint program, GLint location, GLuint *params);
	 void (APIENTRYFP glBindFragDataLocation) (GLuint program, GLuint color, const GLchar *name);
	 GLint (APIENTRYFP glGetFragDataLocation) (GLuint program, const GLchar *name);
	 void (APIENTRYFP glUniform1ui) (GLint location, GLuint v0);
	 void (APIENTRYFP glUniform2ui) (GLint location, GLuint v0, GLuint v1);
	 void (APIENTRYFP glUniform3ui) (GLint location, GLuint v0, GLuint v1, GLuint v2);
	 void (APIENTRYFP glUniform4ui) (GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	 void (APIENTRYFP glUniform1uiv) (GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glUniform2uiv) (GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glUniform3uiv) (GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glUniform4uiv) (GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glTexParameterIiv) (GLenum target, GLenum pname, const GLint *params);
	 void (APIENTRYFP glTexParameterIuiv) (GLenum target, GLenum pname, const GLuint *params);
	 void (APIENTRYFP glGetTexParameterIiv) (GLenum target, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetTexParameterIuiv) (GLenum target, GLenum pname, GLuint *params);
	 void (APIENTRYFP glClearBufferiv) (GLenum buffer, GLint drawbuffer, const GLint *value);
	 void (APIENTRYFP glClearBufferuiv) (GLenum buffer, GLint drawbuffer, const GLuint *value);
	 void (APIENTRYFP glClearBufferfv) (GLenum buffer, GLint drawbuffer, const GLfloat *value);
	 void (APIENTRYFP glClearBufferfi) (GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
	 const GLubyte *(APIENTRYFP glGetStringi) (GLenum name, GLuint index);
	 GLboolean (APIENTRYFP glIsRenderbuffer) (GLuint renderbuffer);
	 void (APIENTRYFP glBindRenderbuffer) (GLenum target, GLuint renderbuffer);
	 void (APIENTRYFP glDeleteRenderbuffers) (GLsizei n, const GLuint *renderbuffers);
	 void (APIENTRYFP glGenRenderbuffers) (GLsizei n, GLuint *renderbuffers);
	 void (APIENTRYFP glRenderbufferStorage) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glGetRenderbufferParameteriv) (GLenum target, GLenum pname, GLint *params);
	 GLboolean (APIENTRYFP glIsFramebuffer) (GLuint framebuffer);
	 void (APIENTRYFP glBindFramebuffer) (GLenum target, GLuint framebuffer);
	 void (APIENTRYFP glDeleteFramebuffers) (GLsizei n, const GLuint *framebuffers);
	 void (APIENTRYFP glGenFramebuffers) (GLsizei n, GLuint *framebuffers);
	 GLenum (APIENTRYFP glCheckFramebufferStatus) (GLenum target);
	 void (APIENTRYFP glFramebufferTexture1D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	 void (APIENTRYFP glFramebufferTexture2D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
	 void (APIENTRYFP glFramebufferTexture3D) (GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level, GLint zoffset);
	 void (APIENTRYFP glFramebufferRenderbuffer) (GLenum target, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	 void (APIENTRYFP glGetFramebufferAttachmentParameteriv) (GLenum target, GLenum attachment, GLenum pname, GLint *params);
	 void (APIENTRYFP glGenerateMipmap) (GLenum target);
	 void (APIENTRYFP glBlitFramebuffer) (GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	 void (APIENTRYFP glRenderbufferStorageMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glFramebufferTextureLayer) (GLenum target, GLenum attachment, GLuint texture, GLint level, GLint layer);
	 void *(APIENTRYFP glMapBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length, GLbitfield access);
	 void (APIENTRYFP glFlushMappedBufferRange) (GLenum target, GLintptr offset, GLsizeiptr length);
	 void (APIENTRYFP glBindVertexArray) (GLuint array);
	 void (APIENTRYFP glDeleteVertexArrays) (GLsizei n, const GLuint *arrays);
	 void (APIENTRYFP glGenVertexArrays) (GLsizei n, GLuint *arrays);
	 GLboolean (APIENTRYFP glIsVertexArray) (GLuint array);
	 void (APIENTRYFP glDrawArraysInstanced) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount);
	 void (APIENTRYFP glDrawElementsInstanced) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount);
	 void (APIENTRYFP glTexBuffer) (GLenum target, GLenum internalformat, GLuint buffer);
	 void (APIENTRYFP glPrimitiveRestartIndex) (GLuint index);
	 void (APIENTRYFP glCopyBufferSubData) (GLenum readTarget, GLenum writeTarget, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	 void (APIENTRYFP glGetUniformIndices) (GLuint program, GLsizei uniformCount, const GLchar *const*uniformNames, GLuint *uniformIndices);
	 void (APIENTRYFP glGetActiveUniformsiv) (GLuint program, GLsizei uniformCount, const GLuint *uniformIndices, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetActiveUniformName) (GLuint program, GLuint uniformIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformName);
	 GLuint (APIENTRYFP glGetUniformBlockIndex) (GLuint program, const GLchar *uniformBlockName);
	 void (APIENTRYFP glGetActiveUniformBlockiv) (GLuint program, GLuint uniformBlockIndex, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetActiveUniformBlockName) (GLuint program, GLuint uniformBlockIndex, GLsizei bufSize, GLsizei *length, GLchar *uniformBlockName);
	 void (APIENTRYFP glUniformBlockBinding) (GLuint program, GLuint uniformBlockIndex, GLuint uniformBlockBinding);
	 void (APIENTRYFP glDrawElementsBaseVertex) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLint basevertex);
	 void (APIENTRYFP glDrawRangeElementsBaseVertex) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices, GLint basevertex);
	 void (APIENTRYFP glDrawElementsInstancedBaseVertex) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex);
	 void (APIENTRYFP glMultiDrawElementsBaseVertex) (GLenum mode, const GLsizei *count, GLenum type, const void *const*indices, GLsizei drawcount, const GLint *basevertex);
	 void (APIENTRYFP glProvokingVertex) (GLenum mode);
	 GLsync (APIENTRYFP glFenceSync) (GLenum condition, GLbitfield flags);
	 GLboolean (APIENTRYFP glIsSync) (GLsync sync);
	 void (APIENTRYFP glDeleteSync) (GLsync sync);
	 GLenum (APIENTRYFP glClientWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout);
	 void (APIENTRYFP glWaitSync) (GLsync sync, GLbitfield flags, GLuint64 timeout);
	 void (APIENTRYFP glGetInteger64v) (GLenum pname, GLint64 *data);
	 void (APIENTRYFP glGetSynciv) (GLsync sync, GLenum pname, GLsizei bufSize, GLsizei *length, GLint *values);
	 void (APIENTRYFP glGetInteger64i_v) (GLenum target, GLuint index, GLint64 *data);
	 void (APIENTRYFP glGetBufferParameteri64v) (GLenum target, GLenum pname, GLint64 *params);
	 void (APIENTRYFP glFramebufferTexture) (GLenum target, GLenum attachment, GLuint texture, GLint level);
	 void (APIENTRYFP glTexImage2DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glTexImage3DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glGetMultisamplefv) (GLenum pname, GLuint index, GLfloat *val);
	 void (APIENTRYFP glSampleMaski) (GLuint maskNumber, GLbitfield mask);
	 void (APIENTRYFP glBindFragDataLocationIndexed) (GLuint program, GLuint colorNumber, GLuint index, const GLchar *name);
	 GLint (APIENTRYFP glGetFragDataIndex) (GLuint program, const GLchar *name);
	 void (APIENTRYFP glGenSamplers) (GLsizei count, GLuint *samplers);
	 void (APIENTRYFP glDeleteSamplers) (GLsizei count, const GLuint *samplers);
	 GLboolean (APIENTRYFP glIsSampler) (GLuint sampler);
	 void (APIENTRYFP glBindSampler) (GLuint unit, GLuint sampler);
	 void (APIENTRYFP glSamplerParameteri) (GLuint sampler, GLenum pname, GLint param);
	 void (APIENTRYFP glSamplerParameteriv) (GLuint sampler, GLenum pname, const GLint *param);
	 void (APIENTRYFP glSamplerParameterf) (GLuint sampler, GLenum pname, GLfloat param);
	 void (APIENTRYFP glSamplerParameterfv) (GLuint sampler, GLenum pname, const GLfloat *param);
	 void (APIENTRYFP glSamplerParameterIiv) (GLuint sampler, GLenum pname, const GLint *param);
	 void (APIENTRYFP glSamplerParameterIuiv) (GLuint sampler, GLenum pname, const GLuint *param);
	 void (APIENTRYFP glGetSamplerParameteriv) (GLuint sampler, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetSamplerParameterIiv) (GLuint sampler, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetSamplerParameterfv) (GLuint sampler, GLenum pname, GLfloat *params);
	 void (APIENTRYFP glGetSamplerParameterIuiv) (GLuint sampler, GLenum pname, GLuint *params);
	 void (APIENTRYFP glQueryCounter) (GLuint id, GLenum target);
	 void (APIENTRYFP glGetQueryObjecti64v) (GLuint id, GLenum pname, GLint64 *params);
	 void (APIENTRYFP glGetQueryObjectui64v) (GLuint id, GLenum pname, GLuint64 *params);
	 void (APIENTRYFP glVertexAttribDivisor) (GLuint index, GLuint divisor);
	 void (APIENTRYFP glVertexAttribP1ui) (GLuint index, GLenum type, GLboolean normalized, GLuint value);
	 void (APIENTRYFP glVertexAttribP1uiv) (GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
	 void (APIENTRYFP glVertexAttribP2ui) (GLuint index, GLenum type, GLboolean normalized, GLuint value);
	 void (APIENTRYFP glVertexAttribP2uiv) (GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
	 void (APIENTRYFP glVertexAttribP3ui) (GLuint index, GLenum type, GLboolean normalized, GLuint value);
	 void (APIENTRYFP glVertexAttribP3uiv) (GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
	 void (APIENTRYFP glVertexAttribP4ui) (GLuint index, GLenum type, GLboolean normalized, GLuint value);
	 void (APIENTRYFP glVertexAttribP4uiv) (GLuint index, GLenum type, GLboolean normalized, const GLuint *value);
	 void (APIENTRYFP glMinSampleShading) (GLfloat value);
	 void (APIENTRYFP glBlendEquationi) (GLuint buf, GLenum mode);
	 void (APIENTRYFP glBlendEquationSeparatei) (GLuint buf, GLenum modeRGB, GLenum modeAlpha);
	 void (APIENTRYFP glBlendFunci) (GLuint buf, GLenum src, GLenum dst);
	 void (APIENTRYFP glBlendFuncSeparatei) (GLuint buf, GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
	 void (APIENTRYFP glDrawArraysIndirect) (GLenum mode, const void *indirect);
	 void (APIENTRYFP glDrawElementsIndirect) (GLenum mode, GLenum type, const void *indirect);
	 void (APIENTRYFP glUniform1d) (GLint location, GLdouble x);
	 void (APIENTRYFP glUniform2d) (GLint location, GLdouble x, GLdouble y);
	 void (APIENTRYFP glUniform3d) (GLint location, GLdouble x, GLdouble y, GLdouble z);
	 void (APIENTRYFP glUniform4d) (GLint location, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	 void (APIENTRYFP glUniform1dv) (GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glUniform2dv) (GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glUniform3dv) (GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glUniform4dv) (GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix2dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix3dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix4dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix2x3dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix2x4dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix3x2dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix3x4dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix4x2dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glUniformMatrix4x3dv) (GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glGetUniformdv) (GLuint program, GLint location, GLdouble *params);
	 GLint (APIENTRYFP glGetSubroutineUniformLocation) (GLuint program, GLenum shadertype, const GLchar *name);
	 GLuint (APIENTRYFP glGetSubroutineIndex) (GLuint program, GLenum shadertype, const GLchar *name);
	 void (APIENTRYFP glGetActiveSubroutineUniformiv) (GLuint program, GLenum shadertype, GLuint index, GLenum pname, GLint *values);
	 void (APIENTRYFP glGetActiveSubroutineUniformName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
	 void (APIENTRYFP glGetActiveSubroutineName) (GLuint program, GLenum shadertype, GLuint index, GLsizei bufsize, GLsizei *length, GLchar *name);
	 void (APIENTRYFP glUniformSubroutinesuiv) (GLenum shadertype, GLsizei count, const GLuint *indices);
	 void (APIENTRYFP glGetUniformSubroutineuiv) (GLenum shadertype, GLint location, GLuint *params);
	 void (APIENTRYFP glGetProgramStageiv) (GLuint program, GLenum shadertype, GLenum pname, GLint *values);
	 void (APIENTRYFP glPatchParameteri) (GLenum pname, GLint value);
	 void (APIENTRYFP glPatchParameterfv) (GLenum pname, const GLfloat *values);
	 void (APIENTRYFP glBindTransformFeedback) (GLenum target, GLuint id);
	 void (APIENTRYFP glDeleteTransformFeedbacks) (GLsizei n, const GLuint *ids);
	 void (APIENTRYFP glGenTransformFeedbacks) (GLsizei n, GLuint *ids);
	 GLboolean (APIENTRYFP glIsTransformFeedback) (GLuint id);
	 void (APIENTRYFP glPauseTransformFeedback) (void);
	 void (APIENTRYFP glResumeTransformFeedback) (void);
	 void (APIENTRYFP glDrawTransformFeedback) (GLenum mode, GLuint id);
	 void (APIENTRYFP glDrawTransformFeedbackStream) (GLenum mode, GLuint id, GLuint stream);
	 void (APIENTRYFP glBeginQueryIndexed) (GLenum target, GLuint index, GLuint id);
	 void (APIENTRYFP glEndQueryIndexed) (GLenum target, GLuint index);
	 void (APIENTRYFP glGetQueryIndexediv) (GLenum target, GLuint index, GLenum pname, GLint *params);
	 void (APIENTRYFP glReleaseShaderCompiler) (void);
	 void (APIENTRYFP glShaderBinary) (GLsizei count, const GLuint *shaders, GLenum binaryformat, const void *binary, GLsizei length);
	 void (APIENTRYFP glGetShaderPrecisionFormat) (GLenum shadertype, GLenum precisiontype, GLint *range, GLint *precision);
	 void (APIENTRYFP glDepthRangef) (GLfloat n, GLfloat f);
	 void (APIENTRYFP glClearDepthf) (GLfloat d);
	 void (APIENTRYFP glGetProgramBinary) (GLuint program, GLsizei bufSize, GLsizei *length, GLenum *binaryFormat, void *binary);
	 void (APIENTRYFP glProgramBinary) (GLuint program, GLenum binaryFormat, const void *binary, GLsizei length);
	 void (APIENTRYFP glProgramParameteri) (GLuint program, GLenum pname, GLint value);
	 void (APIENTRYFP glUseProgramStages) (GLuint pipeline, GLbitfield stages, GLuint program);
	 void (APIENTRYFP glActiveShaderProgram) (GLuint pipeline, GLuint program);
	 GLuint (APIENTRYFP glCreateShaderProgramv) (GLenum type, GLsizei count, const GLchar *const*strings);
	 void (APIENTRYFP glBindProgramPipeline) (GLuint pipeline);
	 void (APIENTRYFP glDeleteProgramPipelines) (GLsizei n, const GLuint *pipelines);
	 void (APIENTRYFP glGenProgramPipelines) (GLsizei n, GLuint *pipelines);
	 GLboolean (APIENTRYFP glIsProgramPipeline) (GLuint pipeline);
	 void (APIENTRYFP glGetProgramPipelineiv) (GLuint pipeline, GLenum pname, GLint *params);
	 void (APIENTRYFP glProgramUniform1i) (GLuint program, GLint location, GLint v0);
	 void (APIENTRYFP glProgramUniform1iv) (GLuint program, GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glProgramUniform1f) (GLuint program, GLint location, GLfloat v0);
	 void (APIENTRYFP glProgramUniform1fv) (GLuint program, GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glProgramUniform1d) (GLuint program, GLint location, GLdouble v0);
	 void (APIENTRYFP glProgramUniform1dv) (GLuint program, GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glProgramUniform1ui) (GLuint program, GLint location, GLuint v0);
	 void (APIENTRYFP glProgramUniform1uiv) (GLuint program, GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glProgramUniform2i) (GLuint program, GLint location, GLint v0, GLint v1);
	 void (APIENTRYFP glProgramUniform2iv) (GLuint program, GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glProgramUniform2f) (GLuint program, GLint location, GLfloat v0, GLfloat v1);
	 void (APIENTRYFP glProgramUniform2fv) (GLuint program, GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glProgramUniform2d) (GLuint program, GLint location, GLdouble v0, GLdouble v1);
	 void (APIENTRYFP glProgramUniform2dv) (GLuint program, GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glProgramUniform2ui) (GLuint program, GLint location, GLuint v0, GLuint v1);
	 void (APIENTRYFP glProgramUniform2uiv) (GLuint program, GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glProgramUniform3i) (GLuint program, GLint location, GLint v0, GLint v1, GLint v2);
	 void (APIENTRYFP glProgramUniform3iv) (GLuint program, GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glProgramUniform3f) (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
	 void (APIENTRYFP glProgramUniform3fv) (GLuint program, GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glProgramUniform3d) (GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2);
	 void (APIENTRYFP glProgramUniform3dv) (GLuint program, GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glProgramUniform3ui) (GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2);
	 void (APIENTRYFP glProgramUniform3uiv) (GLuint program, GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glProgramUniform4i) (GLuint program, GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
	 void (APIENTRYFP glProgramUniform4iv) (GLuint program, GLint location, GLsizei count, const GLint *value);
	 void (APIENTRYFP glProgramUniform4f) (GLuint program, GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
	 void (APIENTRYFP glProgramUniform4fv) (GLuint program, GLint location, GLsizei count, const GLfloat *value);
	 void (APIENTRYFP glProgramUniform4d) (GLuint program, GLint location, GLdouble v0, GLdouble v1, GLdouble v2, GLdouble v3);
	 void (APIENTRYFP glProgramUniform4dv) (GLuint program, GLint location, GLsizei count, const GLdouble *value);
	 void (APIENTRYFP glProgramUniform4ui) (GLuint program, GLint location, GLuint v0, GLuint v1, GLuint v2, GLuint v3);
	 void (APIENTRYFP glProgramUniform4uiv) (GLuint program, GLint location, GLsizei count, const GLuint *value);
	 void (APIENTRYFP glProgramUniformMatrix2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix2x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix3x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix2x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix4x2fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix3x4fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix4x3fv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
	 void (APIENTRYFP glProgramUniformMatrix2x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix3x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix2x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix4x2dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix3x4dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glProgramUniformMatrix4x3dv) (GLuint program, GLint location, GLsizei count, GLboolean transpose, const GLdouble *value);
	 void (APIENTRYFP glValidateProgramPipeline) (GLuint pipeline);
	 void (APIENTRYFP glGetProgramPipelineInfoLog) (GLuint pipeline, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
	 void (APIENTRYFP glVertexAttribL1d) (GLuint index, GLdouble x);
	 void (APIENTRYFP glVertexAttribL2d) (GLuint index, GLdouble x, GLdouble y);
	 void (APIENTRYFP glVertexAttribL3d) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
	 void (APIENTRYFP glVertexAttribL4d) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
	 void (APIENTRYFP glVertexAttribL1dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttribL2dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttribL3dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttribL4dv) (GLuint index, const GLdouble *v);
	 void (APIENTRYFP glVertexAttribLPointer) (GLuint index, GLint size, GLenum type, GLsizei stride, const void *pointer);
	 void (APIENTRYFP glGetVertexAttribLdv) (GLuint index, GLenum pname, GLdouble *params);
	 void (APIENTRYFP glViewportArrayv) (GLuint first, GLsizei count, const GLfloat *v);
	 void (APIENTRYFP glViewportIndexedf) (GLuint index, GLfloat x, GLfloat y, GLfloat w, GLfloat h);
	 void (APIENTRYFP glViewportIndexedfv) (GLuint index, const GLfloat *v);
	 void (APIENTRYFP glScissorArrayv) (GLuint first, GLsizei count, const GLint *v);
	 void (APIENTRYFP glScissorIndexed) (GLuint index, GLint left, GLint bottom, GLsizei width, GLsizei height);
	 void (APIENTRYFP glScissorIndexedv) (GLuint index, const GLint *v);
	 void (APIENTRYFP glDepthRangeArrayv) (GLuint first, GLsizei count, const GLdouble *v);
	 void (APIENTRYFP glDepthRangeIndexed) (GLuint index, GLdouble n, GLdouble f);
	 void (APIENTRYFP glGetFloati_v) (GLenum target, GLuint index, GLfloat *data);
	 void (APIENTRYFP glGetDoublei_v) (GLenum target, GLuint index, GLdouble *data);
	 void (APIENTRYFP glDrawArraysInstancedBaseInstance) (GLenum mode, GLint first, GLsizei count, GLsizei instancecount, GLuint baseinstance);
	 void (APIENTRYFP glDrawElementsInstancedBaseInstance) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLuint baseinstance);
	 void (APIENTRYFP glDrawElementsInstancedBaseVertexBaseInstance) (GLenum mode, GLsizei count, GLenum type, const void *indices, GLsizei instancecount, GLint basevertex, GLuint baseinstance);
	 void (APIENTRYFP glGetInternalformativ) (GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint *params);
	 void (APIENTRYFP glGetActiveAtomicCounterBufferiv) (GLuint program, GLuint bufferIndex, GLenum pname, GLint *params);
	 void (APIENTRYFP glBindImageTexture) (GLuint unit, GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum access, GLenum format);
	 void (APIENTRYFP glMemoryBarrier) (GLbitfield barriers);
	 void (APIENTRYFP glTexStorage1D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width);
	 void (APIENTRYFP glTexStorage2D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glTexStorage3D) (GLenum target, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	 void (APIENTRYFP glDrawTransformFeedbackInstanced) (GLenum mode, GLuint id, GLsizei instancecount);
	 void (APIENTRYFP glDrawTransformFeedbackStreamInstanced) (GLenum mode, GLuint id, GLuint stream, GLsizei instancecount);
	 void (APIENTRYFP glClearBufferData) (GLenum target, GLenum internalformat, GLenum format, GLenum type, const void *data);
	 void (APIENTRYFP glClearBufferSubData) (GLenum target, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	 void (APIENTRYFP glDispatchCompute) (GLuint num_groups_x, GLuint num_groups_y, GLuint num_groups_z);
	 void (APIENTRYFP glDispatchComputeIndirect) (GLintptr indirect);
	 void (APIENTRYFP glCopyImageSubData) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
	 void (APIENTRYFP glFramebufferParameteri) (GLenum target, GLenum pname, GLint param);
	 void (APIENTRYFP glGetFramebufferParameteriv) (GLenum target, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetInternalformati64v) (GLenum target, GLenum internalformat, GLenum pname, GLsizei bufSize, GLint64 *params);
	 void (APIENTRYFP glInvalidateTexSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth);
	 void (APIENTRYFP glInvalidateTexImage) (GLuint texture, GLint level);
	 void (APIENTRYFP glInvalidateBufferSubData) (GLuint buffer, GLintptr offset, GLsizeiptr length);
	 void (APIENTRYFP glInvalidateBufferData) (GLuint buffer);
	 void (APIENTRYFP glInvalidateFramebuffer) (GLenum target, GLsizei numAttachments, const GLenum *attachments);
	 void (APIENTRYFP glInvalidateSubFramebuffer) (GLenum target, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	 void (APIENTRYFP glMultiDrawArraysIndirect) (GLenum mode, const void *indirect, GLsizei drawcount, GLsizei stride);
	 void (APIENTRYFP glMultiDrawElementsIndirect) (GLenum mode, GLenum type, const void *indirect, GLsizei drawcount, GLsizei stride);
	 void (APIENTRYFP glGetProgramInterfaceiv) (GLuint program, GLenum programInterface, GLenum pname, GLint *params);
	 GLuint (APIENTRYFP glGetProgramResourceIndex) (GLuint program, GLenum programInterface, const GLchar *name);
	 void (APIENTRYFP glGetProgramResourceName) (GLuint program, GLenum programInterface, GLuint index, GLsizei bufSize, GLsizei *length, GLchar *name);
	 void (APIENTRYFP glGetProgramResourceiv) (GLuint program, GLenum programInterface, GLuint index, GLsizei propCount, const GLenum *props, GLsizei bufSize, GLsizei *length, GLint *params);
	 GLint (APIENTRYFP glGetProgramResourceLocation) (GLuint program, GLenum programInterface, const GLchar *name);
	 GLint (APIENTRYFP glGetProgramResourceLocationIndex) (GLuint program, GLenum programInterface, const GLchar *name);
	 void (APIENTRYFP glShaderStorageBlockBinding) (GLuint program, GLuint storageBlockIndex, GLuint storageBlockBinding);
	 void (APIENTRYFP glTexBufferRange) (GLenum target, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	 void (APIENTRYFP glTexStorage2DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glTexStorage3DMultisample) (GLenum target, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glTextureView) (GLuint texture, GLenum target, GLuint origtexture, GLenum internalformat, GLuint minlevel, GLuint numlevels, GLuint minlayer, GLuint numlayers);
	 void (APIENTRYFP glBindVertexBuffer) (GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	 void (APIENTRYFP glVertexAttribFormat) (GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
	 void (APIENTRYFP glVertexAttribIFormat) (GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	 void (APIENTRYFP glVertexAttribLFormat) (GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	 void (APIENTRYFP glVertexAttribBinding) (GLuint attribindex, GLuint bindingindex);
	 void (APIENTRYFP glVertexBindingDivisor) (GLuint bindingindex, GLuint divisor);
	 void (APIENTRYFP glDebugMessageControl) (GLenum source, GLenum type, GLenum severity, GLsizei count, const GLuint *ids, GLboolean enabled);
	 void (APIENTRYFP glDebugMessageInsert) (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *buf);
	 void (APIENTRYFP glDebugMessageCallback) (GLDEBUGPROC callback, const void *userParam);
	 GLuint (APIENTRYFP glGetDebugMessageLog) (GLuint count, GLsizei bufSize, GLenum *sources, GLenum *types, GLuint *ids, GLenum *severities, GLsizei *lengths, GLchar *messageLog);
	 void (APIENTRYFP glPushDebugGroup) (GLenum source, GLuint id, GLsizei length, const GLchar *message);
	 void (APIENTRYFP glPopDebugGroup) (void);
	 void (APIENTRYFP glObjectLabel) (GLenum identifier, GLuint name, GLsizei length, const GLchar *label);
	 void (APIENTRYFP glGetObjectLabel) (GLenum identifier, GLuint name, GLsizei bufSize, GLsizei *length, GLchar *label);
	 void (APIENTRYFP glObjectPtrLabel) (const void *ptr, GLsizei length, const GLchar *label);
	 void (APIENTRYFP glGetObjectPtrLabel) (const void *ptr, GLsizei bufSize, GLsizei *length, GLchar *label);
	 void (APIENTRYFP glBufferStorage) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
	 void (APIENTRYFP glClearTexImage) (GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
	 void (APIENTRYFP glClearTexSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);
	 void (APIENTRYFP glBindBuffersBase) (GLenum target, GLuint first, GLsizei count, const GLuint *buffers);
	 void (APIENTRYFP glBindBuffersRange) (GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes);
	 void (APIENTRYFP glBindTextures) (GLuint first, GLsizei count, const GLuint *textures);
	 void (APIENTRYFP glBindSamplers) (GLuint first, GLsizei count, const GLuint *samplers);
	 void (APIENTRYFP glBindImageTextures) (GLuint first, GLsizei count, const GLuint *textures);
	 void (APIENTRYFP glBindVertexBuffers) (GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);
	 void (APIENTRYFP glClipControl) (GLenum origin, GLenum depth);
	 void (APIENTRYFP glCreateTransformFeedbacks) (GLsizei n, GLuint *ids);
	 void (APIENTRYFP glTransformFeedbackBufferBase) (GLuint xfb, GLuint index, GLuint buffer);
	 void (APIENTRYFP glTransformFeedbackBufferRange) (GLuint xfb, GLuint index, GLuint buffer, GLintptr offset, GLsizeiptr size);
	 void (APIENTRYFP glGetTransformFeedbackiv) (GLuint xfb, GLenum pname, GLint *param);
	 void (APIENTRYFP glGetTransformFeedbacki_v) (GLuint xfb, GLenum pname, GLuint index, GLint *param);
	 void (APIENTRYFP glGetTransformFeedbacki64_v) (GLuint xfb, GLenum pname, GLuint index, GLint64 *param);
	 void (APIENTRYFP glCreateBuffers) (GLsizei n, GLuint *buffers);
	 void (APIENTRYFP glNamedBufferStorage) (GLuint buffer, GLsizeiptr size, const void *data, GLbitfield flags);
	 void (APIENTRYFP glNamedBufferData) (GLuint buffer, GLsizeiptr size, const void *data, GLenum usage);
	 void (APIENTRYFP glNamedBufferSubData) (GLuint buffer, GLintptr offset, GLsizeiptr size, const void *data);
	 void (APIENTRYFP glCopyNamedBufferSubData) (GLuint readBuffer, GLuint writeBuffer, GLintptr readOffset, GLintptr writeOffset, GLsizeiptr size);
	 void (APIENTRYFP glClearNamedBufferData) (GLuint buffer, GLenum internalformat, GLenum format, GLenum type, const void *data);
	 void (APIENTRYFP glClearNamedBufferSubData) (GLuint buffer, GLenum internalformat, GLintptr offset, GLsizeiptr size, GLenum format, GLenum type, const void *data);
	 void *(APIENTRYFP glMapNamedBuffer) (GLuint buffer, GLenum access);
	 void *(APIENTRYFP glMapNamedBufferRange) (GLuint buffer, GLintptr offset, GLsizeiptr length, GLbitfield access);
	 GLboolean (APIENTRYFP glUnmapNamedBuffer) (GLuint buffer);
	 void (APIENTRYFP glFlushMappedNamedBufferRange) (GLuint buffer, GLintptr offset, GLsizeiptr length);
	 void (APIENTRYFP glGetNamedBufferParameteriv) (GLuint buffer, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetNamedBufferParameteri64v) (GLuint buffer, GLenum pname, GLint64 *params);
	 void (APIENTRYFP glGetNamedBufferPointerv) (GLuint buffer, GLenum pname, void **params);
	 void (APIENTRYFP glGetNamedBufferSubData) (GLuint buffer, GLintptr offset, GLsizeiptr size, void *data);
	 void (APIENTRYFP glCreateFramebuffers) (GLsizei n, GLuint *framebuffers);
	 void (APIENTRYFP glNamedFramebufferRenderbuffer) (GLuint framebuffer, GLenum attachment, GLenum renderbuffertarget, GLuint renderbuffer);
	 void (APIENTRYFP glNamedFramebufferParameteri) (GLuint framebuffer, GLenum pname, GLint param);
	 void (APIENTRYFP glNamedFramebufferTexture) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level);
	 void (APIENTRYFP glNamedFramebufferTextureLayer) (GLuint framebuffer, GLenum attachment, GLuint texture, GLint level, GLint layer);
	 void (APIENTRYFP glNamedFramebufferDrawBuffer) (GLuint framebuffer, GLenum buf);
	 void (APIENTRYFP glNamedFramebufferDrawBuffers) (GLuint framebuffer, GLsizei n, const GLenum *bufs);
	 void (APIENTRYFP glNamedFramebufferReadBuffer) (GLuint framebuffer, GLenum src);
	 void (APIENTRYFP glInvalidateNamedFramebufferData) (GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments);
	 void (APIENTRYFP glInvalidateNamedFramebufferSubData) (GLuint framebuffer, GLsizei numAttachments, const GLenum *attachments, GLint x, GLint y, GLsizei width, GLsizei height);
	 void (APIENTRYFP glClearNamedFramebufferiv) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLint *value);
	 void (APIENTRYFP glClearNamedFramebufferuiv) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLuint *value);
	 void (APIENTRYFP glClearNamedFramebufferfv) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, const GLfloat *value);
	 void (APIENTRYFP glClearNamedFramebufferfi) (GLuint framebuffer, GLenum buffer, GLint drawbuffer, GLfloat depth, GLint stencil);
	 void (APIENTRYFP glBlitNamedFramebuffer) (GLuint readFramebuffer, GLuint drawFramebuffer, GLint srcX0, GLint srcY0, GLint srcX1, GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1, GLbitfield mask, GLenum filter);
	 GLenum (APIENTRYFP glCheckNamedFramebufferStatus) (GLuint framebuffer, GLenum target);
	 void (APIENTRYFP glGetNamedFramebufferParameteriv) (GLuint framebuffer, GLenum pname, GLint *param);
	 void (APIENTRYFP glGetNamedFramebufferAttachmentParameteriv) (GLuint framebuffer, GLenum attachment, GLenum pname, GLint *params);
	 void (APIENTRYFP glCreateRenderbuffers) (GLsizei n, GLuint *renderbuffers);
	 void (APIENTRYFP glNamedRenderbufferStorage) (GLuint renderbuffer, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glNamedRenderbufferStorageMultisample) (GLuint renderbuffer, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glGetNamedRenderbufferParameteriv) (GLuint renderbuffer, GLenum pname, GLint *params);
	 void (APIENTRYFP glCreateTextures) (GLenum target, GLsizei n, GLuint *textures);
	 void (APIENTRYFP glTextureBuffer) (GLuint texture, GLenum internalformat, GLuint buffer);
	 void (APIENTRYFP glTextureBufferRange) (GLuint texture, GLenum internalformat, GLuint buffer, GLintptr offset, GLsizeiptr size);
	 void (APIENTRYFP glTextureStorage1D) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width);
	 void (APIENTRYFP glTextureStorage2D) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height);
	 void (APIENTRYFP glTextureStorage3D) (GLuint texture, GLsizei levels, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth);
	 void (APIENTRYFP glTextureStorage2DMultisample) (GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glTextureStorage3DMultisample) (GLuint texture, GLsizei samples, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLboolean fixedsamplelocations);
	 void (APIENTRYFP glTextureSubImage1D) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const void *pixels);
	 void (APIENTRYFP glTextureSubImage2D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const void *pixels);
	 void (APIENTRYFP glTextureSubImage3D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *pixels);
	 void (APIENTRYFP glCompressedTextureSubImage1D) (GLuint texture, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTextureSubImage2D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCompressedTextureSubImage3D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const void *data);
	 void (APIENTRYFP glCopyTextureSubImage1D) (GLuint texture, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
	 void (APIENTRYFP glCopyTextureSubImage2D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	 void (APIENTRYFP glCopyTextureSubImage3D) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
	 void (APIENTRYFP glTextureParameterf) (GLuint texture, GLenum pname, GLfloat param);
	 void (APIENTRYFP glTextureParameterfv) (GLuint texture, GLenum pname, const GLfloat *param);
	 void (APIENTRYFP glTextureParameteri) (GLuint texture, GLenum pname, GLint param);
	 void (APIENTRYFP glTextureParameterIiv) (GLuint texture, GLenum pname, const GLint *params);
	 void (APIENTRYFP glTextureParameterIuiv) (GLuint texture, GLenum pname, const GLuint *params);
	 void (APIENTRYFP glTextureParameteriv) (GLuint texture, GLenum pname, const GLint *param);
	 void (APIENTRYFP glGenerateTextureMipmap) (GLuint texture);
	 void (APIENTRYFP glBindTextureUnit) (GLuint unit, GLuint texture);
	 void (APIENTRYFP glGetTextureImage) (GLuint texture, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	 void (APIENTRYFP glGetCompressedTextureImage) (GLuint texture, GLint level, GLsizei bufSize, void *pixels);
	 void (APIENTRYFP glGetTextureLevelParameterfv) (GLuint texture, GLint level, GLenum pname, GLfloat *params);
	 void (APIENTRYFP glGetTextureLevelParameteriv) (GLuint texture, GLint level, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetTextureParameterfv) (GLuint texture, GLenum pname, GLfloat *params);
	 void (APIENTRYFP glGetTextureParameterIiv) (GLuint texture, GLenum pname, GLint *params);
	 void (APIENTRYFP glGetTextureParameterIuiv) (GLuint texture, GLenum pname, GLuint *params);
	 void (APIENTRYFP glGetTextureParameteriv) (GLuint texture, GLenum pname, GLint *params);
	 void (APIENTRYFP glCreateVertexArrays) (GLsizei n, GLuint *arrays);
	 void (APIENTRYFP glDisableVertexArrayAttrib) (GLuint vaobj, GLuint index);
	 void (APIENTRYFP glEnableVertexArrayAttrib) (GLuint vaobj, GLuint index);
	 void (APIENTRYFP glVertexArrayElementBuffer) (GLuint vaobj, GLuint buffer);
	 void (APIENTRYFP glVertexArrayVertexBuffer) (GLuint vaobj, GLuint bindingindex, GLuint buffer, GLintptr offset, GLsizei stride);
	 void (APIENTRYFP glVertexArrayVertexBuffers) (GLuint vaobj, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);
	 void (APIENTRYFP glVertexArrayAttribBinding) (GLuint vaobj, GLuint attribindex, GLuint bindingindex);
	 void (APIENTRYFP glVertexArrayAttribFormat) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLboolean normalized, GLuint relativeoffset);
	 void (APIENTRYFP glVertexArrayAttribIFormat) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	 void (APIENTRYFP glVertexArrayAttribLFormat) (GLuint vaobj, GLuint attribindex, GLint size, GLenum type, GLuint relativeoffset);
	 void (APIENTRYFP glVertexArrayBindingDivisor) (GLuint vaobj, GLuint bindingindex, GLuint divisor);
	 void (APIENTRYFP glGetVertexArrayiv) (GLuint vaobj, GLenum pname, GLint *param);
	 void (APIENTRYFP glGetVertexArrayIndexediv) (GLuint vaobj, GLuint index, GLenum pname, GLint *param);
	 void (APIENTRYFP glGetVertexArrayIndexed64iv) (GLuint vaobj, GLuint index, GLenum pname, GLint64 *param);
	 void (APIENTRYFP glCreateSamplers) (GLsizei n, GLuint *samplers);
	 void (APIENTRYFP glCreateProgramPipelines) (GLsizei n, GLuint *pipelines);
	 void (APIENTRYFP glCreateQueries) (GLenum target, GLsizei n, GLuint *ids);
	 void (APIENTRYFP glGetQueryBufferObjecti64v) (GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	 void (APIENTRYFP glGetQueryBufferObjectiv) (GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	 void (APIENTRYFP glGetQueryBufferObjectui64v) (GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	 void (APIENTRYFP glGetQueryBufferObjectuiv) (GLuint id, GLuint buffer, GLenum pname, GLintptr offset);
	 void (APIENTRYFP glMemoryBarrierByRegion) (GLbitfield barriers);
	 void (APIENTRYFP glGetTextureSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	 void (APIENTRYFP glGetCompressedTextureSubImage) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei bufSize, void *pixels);
	 GLenum (APIENTRYFP glGetGraphicsResetStatus) (void);
	 void (APIENTRYFP glGetnCompressedTexImage) (GLenum target, GLint lod, GLsizei bufSize, void *pixels);
	 void (APIENTRYFP glGetnTexImage) (GLenum target, GLint level, GLenum format, GLenum type, GLsizei bufSize, void *pixels);
	 void (APIENTRYFP glGetnUniformdv) (GLuint program, GLint location, GLsizei bufSize, GLdouble *params);
	 void (APIENTRYFP glGetnUniformfv) (GLuint program, GLint location, GLsizei bufSize, GLfloat *params);
	 void (APIENTRYFP glGetnUniformiv) (GLuint program, GLint location, GLsizei bufSize, GLint *params);
	 void (APIENTRYFP glGetnUniformuiv) (GLuint program, GLint location, GLsizei bufSize, GLuint *params);
	 void (APIENTRYFP glReadnPixels) (GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLsizei bufSize, void *data);
	 void (APIENTRYFP glTextureBarrier) (void);
#endif
