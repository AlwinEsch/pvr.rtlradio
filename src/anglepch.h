//---------------------------------------------------------------------------
// Copyright (c) 2020-2022 Michael G. Brehm
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//---------------------------------------------------------------------------


#ifndef __ANGLEPCH_H_
#define __ANGLEPCH_H_
#pragma once

#include <kodi/gui/gl/GL.h>
/*
#ifndef D3DERR_OUTOFVIDEOMEMORY
#define D3DERR_OUTOFVIDEOMEMORY E_FAIL
#endif

// ANGLE takes a terribly long time to build; throwing as many headers as possible
// into a forced precompiled header makes a tremendous difference ...

#include "common/Color.h"
#include "common/FastVector.h"
#include "common/FixedVector.h"
#include "common/MemoryBuffer.h"
#include "common/Optional.h"
#include "common/PackedEGLEnums_autogen.h"
#include "common/PackedEnums.h"
#include "common/PackedGLEnums_autogen.h"
#include "common/PoolAlloc.h"
#include "common/aligned_memory.h"
#include "common/android_util.h"
#include "common/angleutils.h"
#include "common/bitset_utils.h"
#include "common/debug.h"
#include "common/event_tracer.h"
#include "common/hash_utils.h"
#include "common/mathutil.h"
#include "common/matrix_utils.h"
#include "common/platform.h"
#include "common/string_utils.h"
#include "common/system_utils.h"
#include "common/tls.h"
#include "common/utilities.h"
#include "common/vector_utils.h"
#include "common/version.h"
#include "compiler/preprocessor/DiagnosticsBase.h"
#include "compiler/preprocessor/DirectiveHandlerBase.h"
#include "compiler/preprocessor/DirectiveParser.h"
#include "compiler/preprocessor/ExpressionParser.h"
#include "compiler/preprocessor/Input.h"
#include "compiler/preprocessor/Lexer.h"
#include "compiler/preprocessor/Macro.h"
#include "compiler/preprocessor/MacroExpander.h"
#include "compiler/preprocessor/Preprocessor.h"
#include "compiler/preprocessor/SourceLocation.h"
#include "compiler/preprocessor/Token.h"
#include "compiler/preprocessor/Tokenizer.h"
#include "compiler/preprocessor/numeric_lex.h"
#include "compiler/translator/ASTMetadataHLSL.h"
#include "compiler/translator/AtomicCounterFunctionHLSL.h"
#include "compiler/translator/BaseTypes.h"
#include "compiler/translator/BuiltInFunctionEmulator.h"
#include "compiler/translator/BuiltInFunctionEmulatorGLSL.h"
#include "compiler/translator/BuiltInFunctionEmulatorHLSL.h"
#include "compiler/translator/CallDAG.h"
#include "compiler/translator/CollectVariables.h"
#include "compiler/translator/Common.h"
#include "compiler/translator/Compiler.h"
#include "compiler/translator/ConstantUnion.h"
#include "compiler/translator/Declarator.h"
#include "compiler/translator/Diagnostics.h"
#include "compiler/translator/DirectiveHandler.h"
#include "compiler/translator/ExtensionBehavior.h"
#include "compiler/translator/ExtensionGLSL.h"
#include "compiler/translator/FlagStd140Structs.h"
#include "compiler/translator/FunctionLookup.h"
#include "compiler/translator/HashNames.h"
#include "compiler/translator/ImageFunctionHLSL.h"
#include "compiler/translator/ImmutableString.h"
#include "compiler/translator/ImmutableStringBuilder.h"
#include "compiler/translator/InfoSink.h"
#include "compiler/translator/Initialize.h"
#include "compiler/translator/InitializeDll.h"
#include "compiler/translator/InitializeGlobals.h"
#include "compiler/translator/IntermNode.h"
#include "compiler/translator/IsASTDepthBelowLimit.h"
#include "compiler/translator/Operator.h"
#include "compiler/translator/OutputESSL.h"
#include "compiler/translator/OutputGLSL.h"
#include "compiler/translator/OutputGLSLBase.h"
#include "compiler/translator/OutputHLSL.h"
#include "compiler/translator/OutputTree.h"
#include "compiler/translator/ParseContext.h"
#include "compiler/translator/blocklayout.h"
#include "compiler/translator/blocklayoutHLSL.h"
#include "compiler/translator/glslang.h"
#include "compiler/translator/length_limits.h"
//#include "compiler/translator/ParseContext_autogen.h"
#include "compiler/translator/PoolAlloc.h"
#include "compiler/translator/Pragma.h"
#include "compiler/translator/QualifierTypes.h"
#include "compiler/translator/ResourcesHLSL.h"
#include "compiler/translator/Severity.h"
#include "compiler/translator/ShaderStorageBlockFunctionHLSL.h"
#include "compiler/translator/ShaderStorageBlockOutputHLSL.h"
#include "compiler/translator/StaticType.h"
#include "compiler/translator/StructureHLSL.h"
#include "compiler/translator/Symbol.h"
#include "compiler/translator/SymbolTable.h"
#include "compiler/translator/SymbolTable_autogen.h"
#include "compiler/translator/SymbolUniqueId.h"
#include "compiler/translator/TextureFunctionHLSL.h"
#include "compiler/translator/TranslatorESSL.h"
#include "compiler/translator/TranslatorGLSL.h"
#include "compiler/translator/TranslatorHLSL.h"
#include "compiler/translator/Types.h"
//#include "compiler/translator/util.h"
#include "compiler/translator/UtilsHLSL.h"
#include "compiler/translator/ValidateAST.h"
#include "compiler/translator/ValidateGlobalInitializer.h"
#include "compiler/translator/ValidateLimitations.h"
#include "compiler/translator/ValidateMaxParameters.h"
#include "compiler/translator/ValidateOutputs.h"
#include "compiler/translator/ValidateSwitch.h"
#include "compiler/translator/ValidateVaryingLocations.h"
#include "compiler/translator/VariablePacker.h"
#include "compiler/translator/VersionGLSL.h"

// Skip compiler/translator/tree_ops/*.h; causes problems
// 
// Skip compiler/translator/tree_util/*.h; causes problems

#include "libANGLE/AttributeMap.h"
#include "libANGLE/BinaryStream.h"
#include "libANGLE/BlobCache.h"
#include "libANGLE/Buffer.h"
#include "libANGLE/Caps.h"
#include "libANGLE/Compiler.h"
#include "libANGLE/Config.h"
#include "libANGLE/Constants.h"
#include "libANGLE/Context.h"
#include "libANGLE/Context.inl.h"
#include "libANGLE/Context_gles_1_0_autogen.h"
#include "libANGLE/Debug.h"
#include "libANGLE/Device.h"
#include "libANGLE/Display.h"
#include "libANGLE/EGLSync.h"
#include "libANGLE/Error.h"
#include "libANGLE/ErrorStrings.h"
#include "libANGLE/Fence.h"
#include "libANGLE/Framebuffer.h"
#include "libANGLE/FramebufferAttachment.h"
#include "libANGLE/GLES1Renderer.h"
#include "libANGLE/GLES1State.h"
#include "libANGLE/HandleAllocator.h"
#include "libANGLE/HandleRangeAllocator.h"
#include "libANGLE/Image.h"
#include "libANGLE/ImageIndex.h"
#include "libANGLE/IndexRangeCache.h"
#include "libANGLE/LoggingAnnotator.h"
#include "libANGLE/MemoryObject.h"
#include "libANGLE/MemoryProgramCache.h"
#include "libANGLE/Observer.h"
#include "libANGLE/Overlay.h"
#include "libANGLE/OverlayWidgets.h"
#include "libANGLE/Program.h"
#include "libANGLE/ProgramLinkedResources.h"
#include "libANGLE/ProgramPipeline.h"
#include "libANGLE/Query.h"
#include "libANGLE/RefCountObject.h"
#include "libANGLE/Renderbuffer.h"
#include "libANGLE/ResourceManager.h"
#include "libANGLE/ResourceMap.h"
#include "libANGLE/Sampler.h"
#include "libANGLE/Semaphore.h"
#include "libANGLE/Shader.h"
#include "libANGLE/SizedMRUCache.h"
#include "libANGLE/State.h"
#include "libANGLE/Stream.h"
#include "libANGLE/Surface.h"
#include "libANGLE/Texture.h"
#include "libANGLE/Thread.h"
#include "libANGLE/TransformFeedback.h"
#include "libANGLE/Uniform.h"
#include "libANGLE/VaryingPacking.h"
#include "libANGLE/Version.h"
#include "libANGLE/VertexArray.h"
#include "libANGLE/VertexAttribute.h"
#include "libANGLE/WorkerThread.h"
#include "libANGLE/angletypes.h"
#include "libANGLE/features.h"
#include "libANGLE/formatutils.h"
#include "libANGLE/histogram_macros.h"
#include "libANGLE/queryconversions.h"
#include "libANGLE/queryutils.h"
#include "libANGLE/renderer/BufferImpl.h"
#include "libANGLE/renderer/CompilerImpl.h"
#include "libANGLE/renderer/ContextImpl.h"
#include "libANGLE/renderer/DeviceImpl.h"
#include "libANGLE/renderer/DisplayImpl.h"
#include "libANGLE/renderer/EGLImplFactory.h"
#include "libANGLE/renderer/EGLSyncImpl.h"
#include "libANGLE/renderer/FenceNVImpl.h"
#include "libANGLE/renderer/Format.h"
#include "libANGLE/renderer/FormatID_autogen.h"
#include "libANGLE/renderer/FramebufferAttachmentObjectImpl.h"
#include "libANGLE/renderer/FramebufferImpl.h"
#include "libANGLE/renderer/GLImplFactory.h"
#include "libANGLE/renderer/ImageImpl.h"
#include "libANGLE/renderer/PathImpl.h"
#include "libANGLE/renderer/ProgramImpl.h"
#include "libANGLE/renderer/ProgramPipelineImpl.h"
#include "libANGLE/renderer/QueryImpl.h"
#include "libANGLE/renderer/RenderTargetCache.h"
#include "libANGLE/renderer/RenderbufferImpl.h"
#include "libANGLE/renderer/SamplerImpl.h"
#include "libANGLE/renderer/SemaphoreImpl.h"
#include "libANGLE/renderer/ShaderImpl.h"
#include "libANGLE/renderer/StreamProducerImpl.h"
#include "libANGLE/renderer/SurfaceImpl.h"
#include "libANGLE/renderer/SyncImpl.h"
#include "libANGLE/renderer/TextureImpl.h"
#include "libANGLE/renderer/TransformFeedbackImpl.h"
#include "libANGLE/renderer/VertexArrayImpl.h"
#include "libANGLE/renderer/copyvertex.h"
#include "libANGLE/renderer/d3d/BufferD3D.h"
#include "libANGLE/renderer/d3d/CompilerD3D.h"
#include "libANGLE/renderer/d3d/ContextD3D.h"
#include "libANGLE/renderer/d3d/DeviceD3D.h"
#include "libANGLE/renderer/d3d/DisplayD3D.h"
#include "libANGLE/renderer/d3d/DynamicHLSL.h"
#include "libANGLE/renderer/d3d/DynamicImage2DHLSL.h"
#include "libANGLE/renderer/d3d/EGLImageD3D.h"
#include "libANGLE/renderer/d3d/FramebufferD3D.h"
#include "libANGLE/renderer/d3d/HLSLCompiler.h"
#include "libANGLE/renderer/d3d/ImageD3D.h"
#include "libANGLE/renderer/d3d/IndexBuffer.h"
#include "libANGLE/renderer/d3d/IndexDataManager.h"
#include "libANGLE/renderer/d3d/NativeWindowD3D.h"
#include "libANGLE/renderer/d3d/ProgramD3D.h"
#include "libANGLE/renderer/d3d/RenderTargetD3D.h"
#include "libANGLE/renderer/d3d/RenderbufferD3D.h"
#include "libANGLE/renderer/d3d/RendererD3D.h"
#include "libANGLE/renderer/d3d/SamplerD3D.h"
#include "libANGLE/renderer/d3d/ShaderD3D.h"
#include "libANGLE/renderer/d3d/ShaderExecutableD3D.h"
#include "libANGLE/renderer/d3d/SurfaceD3D.h"
#include "libANGLE/renderer/d3d/SwapChainD3D.h"
#include "libANGLE/renderer/d3d/TextureD3D.h"
#include "libANGLE/renderer/d3d/TextureStorage.h"
#include "libANGLE/renderer/d3d/VertexBuffer.h"
#include "libANGLE/renderer/d3d/VertexDataManager.h"
#include "libANGLE/renderer/d3d/d3d11/Blit11.h"
#include "libANGLE/renderer/d3d/d3d11/Buffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Clear11.h"
#include "libANGLE/renderer/d3d/d3d11/Context11.h"
#include "libANGLE/renderer/d3d/d3d11/DebugAnnotator11.h"
#include "libANGLE/renderer/d3d/d3d11/ExternalImageSiblingImpl11.h"
#include "libANGLE/renderer/d3d/d3d11/Fence11.h"
#include "libANGLE/renderer/d3d/d3d11/Framebuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/Image11.h"
#include "libANGLE/renderer/d3d/d3d11/IndexBuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/InputLayoutCache.h"
#include "libANGLE/renderer/d3d/d3d11/MappedSubresourceVerifier11.h"
#include "libANGLE/renderer/d3d/d3d11/NativeWindow11.h"
#include "libANGLE/renderer/d3d/d3d11/PixelTransfer11.h"
#include "libANGLE/renderer/d3d/d3d11/Program11.h"
#include "libANGLE/renderer/d3d/d3d11/ProgramPipeline11.h"
#include "libANGLE/renderer/d3d/d3d11/Query11.h"
#include "libANGLE/renderer/d3d/d3d11/RenderStateCache.h"
#include "libANGLE/renderer/d3d/d3d11/RenderTarget11.h"
#include "libANGLE/renderer/d3d/d3d11/Renderer11.h"
#include "libANGLE/renderer/d3d/d3d11/ResourceManager11.h"
#include "libANGLE/renderer/d3d/d3d11/ShaderExecutable11.h"
#include "libANGLE/renderer/d3d/d3d11/StateManager11.h"
#include "libANGLE/renderer/d3d/d3d11/StreamProducerD3DTexture.h"
#include "libANGLE/renderer/d3d/d3d11/SwapChain11.h"
#include "libANGLE/renderer/d3d/d3d11/TextureStorage11.h"
#include "libANGLE/renderer/d3d/d3d11/TransformFeedback11.h"
#include "libANGLE/renderer/d3d/d3d11/Trim11.h"
#include "libANGLE/renderer/d3d/d3d11/VertexArray11.h"
#include "libANGLE/renderer/d3d/d3d11/VertexBuffer11.h"
#include "libANGLE/renderer/d3d/d3d11/dxgi_support_table.h"
#include "libANGLE/renderer/d3d/d3d11/formatutils11.h"
#include "libANGLE/renderer/d3d/d3d11/renderer11_utils.h"
#include "libANGLE/renderer/d3d/d3d11/texture_format_table.h"
#include "libANGLE/renderer/d3d/d3d11/texture_format_table_utils.h"
#include "libANGLE/renderer/d3d/formatutilsD3D.h"
#include "libANGLE/renderer/driver_utils.h"
#include "libANGLE/renderer/load_functions_table.h"
#include "libANGLE/renderer/renderer_utils.h"
#include "libANGLE/validationEGL.h"
#include "libANGLE/validationES.h"
#include "libANGLE/validationES1.h"
#include "libANGLE/validationES1_autogen.h"
#include "libANGLE/validationES2.h"
#include "libANGLE/validationES2_autogen.h"
#include "libANGLE/validationES3.h"
#include "libANGLE/validationES31.h"
#include "libANGLE/validationES31_autogen.h"
#include "libANGLE/validationES3_autogen.h"
#include "libANGLE/validationESEXT.h"
#include "libANGLE/validationESEXT_autogen.h"

*/
//---------------------------------------------------------------------------

#endif // __ANGLEPCH_H_
