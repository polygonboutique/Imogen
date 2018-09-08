#include "ImApp.h"
#include "Evaluation.h"
#include <vector>
#include <algorithm>
#include <assert.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "hdrloader.h"
#include <map>
#include <fstream>
#include <streambuf>


extern int Log(const char *szFormat, ...);
static const int SemUV0 = 0;

void RenderTarget::bindAsTarget() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glViewport(0, 0, mWidth, mHeight);
}

void RenderTarget::destroy()
{
	if (mGLTexID)
		glDeleteTextures(1, &mGLTexID);
	if (fbo)
		glDeleteFramebuffers(1, &fbo);
	if (depthbuffer)
		glDeleteRenderbuffers(1, &depthbuffer);
	fbo = depthbuffer = 0;
	mWidth = mHeight = 0;
	mGLTexID = 0;
}

void RenderTarget::clear()
{
	glClear(GL_COLOR_BUFFER_BIT | (depthbuffer ? GL_DEPTH_BUFFER_BIT : 0));
}

void RenderTarget::initBuffer(int width, int height, bool hasZBuffer)
{
	if ((width == mWidth) && (mHeight == height) && (!(hasZBuffer ^ (depthbuffer != 0))))
		return;
	destroy();

	mWidth = width;
	mHeight = height;

	//LOG("New Target %dx%d\n", width, height);

	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	// diffuse
	glGenTextures(1, &mGLTexID);
	glBindTexture(GL_TEXTURE_2D, mGLTexID);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP, GL_CLAMP, GL_TEXTURE_2D);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mGLTexID, 0);
	/*
	if (hasZBuffer)
	{
		// Z
		glGenTextures(1, &mGLTexID2);
		glBindTexture(GL_TEXTURE_2D, mGLTexID2);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
		TexParam(GL_NEAREST, GL_NEAREST, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_TEXTURE_2D);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, mGLTexID2, 0);
	}
	*/
	static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(sizeof(DrawBuffers) / sizeof(GLenum), DrawBuffers);

	checkFBO();
}

void RenderTarget::checkFBO()
{
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);

	int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	switch (status)
	{
	case GL_FRAMEBUFFER_COMPLETE:
		//Log("Framebuffer complete.\n");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
		Log("[ERROR] Framebuffer incomplete: Attachment is NOT complete.");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
		Log("[ERROR] Framebuffer incomplete: No image is attached to FBO.");
		break;
/*
	case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
		Log("[ERROR] Framebuffer incomplete: Attached images have different dimensions.");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
		Log("[ERROR] Framebuffer incomplete: Color attached images have different internal formats.");
		break;
*/
	case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
		Log("[ERROR] Framebuffer incomplete: Draw buffer.\n");
		break;

	case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
		Log("[ERROR] Framebuffer incomplete: Read buffer.\n");
		break;

	case GL_FRAMEBUFFER_UNSUPPORTED:
		Log("[ERROR] Unsupported by FBO implementation.\n");
		break;

	default:
		Log("[ERROR] Unknow error.\n");
		break;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void FullScreenTriangle::Init()
{
	TextureID fsVA;

	float fsVts[] = { 0.f,0.f, 2.f,0.f, 0.f,2.f };
	glGenBuffers(1, &fsVA);
	glBindBuffer(GL_ARRAY_BUFFER, fsVA);
	glBufferData(GL_ARRAY_BUFFER, 3 * sizeof(float) * 2, fsVts, GL_STATIC_DRAW);

	glGenVertexArrays(1, &mGLFullScreenVertexArrayName);
	glBindVertexArray(mGLFullScreenVertexArrayName);
	glBindBuffer(GL_ARRAY_BUFFER, fsVA);
	glVertexAttribPointer(SemUV0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(SemUV0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void FullScreenTriangle::Render()
{
	glBindVertexArray(mGLFullScreenVertexArrayName);
	glDrawArrays(GL_TRIANGLES, 0, 3);
	glBindVertexArray(0);
}

unsigned int LoadShader(const std::string &shaderString, const char *fileName)
{
	TextureID programObject = glCreateProgram();
	if (programObject == 0)
		return 0;

	GLint compiled;
	const char *shaderTypeStrings[] = { "\n#version 330 core\n#define VERTEX_SHADER\n", "\n#version 330 core\n#define FRAGMENT_SHADER\n" };
	TextureID shaderTypes[] = { GL_VERTEX_SHADER, GL_FRAGMENT_SHADER };
	TextureID compiledShader[2];

	for (int i = 0; i<2; i++)
	{
		// Create the shader object
		int shader = glCreateShader(shaderTypes[i]);

		if (shader == 0)
			return false;

		int stringsCount = 2;
		const char ** strings = (const char**)malloc(sizeof(char*) * stringsCount); //new const char*[stringsCount];
		int * stringLength = (int*)malloc(sizeof(int) * stringsCount); //new int[stringsCount];
		strings[0] = shaderTypeStrings[i];
		stringLength[0] = strlen(shaderTypeStrings[i]);
		strings[stringsCount - 1] = shaderString.c_str();
		stringLength[stringsCount - 1] = shaderString.length();

		// Load and compile the shader source
		glShaderSource(shader, stringsCount, strings, stringLength);
		glCompileShader(shader);


		free(stringLength);
		free(strings);

		// Check the compile status
		glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
		if (compiled == 0)
		{
			GLint info_len = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);
			if (info_len > 1)
			{
				char* info_log = (char*)malloc(sizeof(char) * info_len);
				glGetShaderInfoLog(shader, info_len, NULL, info_log);
				Log("Error compiling shader: %s \n", fileName);
				Log(info_log);
				Log("\n");
				free(info_log);
			}
			glDeleteShader(shader);
			return 0;
		}
		compiledShader[i] = shader;
	}



	GLint linked;

	for (int i = 0; i<2; i++)
		glAttachShader(programObject, compiledShader[i]);


	// Link the program
	glLinkProgram(programObject);

	//glBindAttribLocation(programObject, SemPosition, "inPosition");
	//glBindAttribLocation(programObject, SemNormal, "inNormal");
	glBindAttribLocation(programObject, SemUV0, "inUV");

	//glBindAttribLocation(programObject, SemInstanceMatrix, "inInstanceMatrix");
	//glBindAttribLocation(programObject, SemInstanceColor, "inInstanceColor");


	// Check the link status
	glGetProgramiv(programObject, GL_LINK_STATUS, &linked);
	if (linked == 0)
	{
		GLint info_len = 0;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &info_len);
		if (info_len > 1)
		{
			char* info_log = (char*)malloc(sizeof(char) * info_len);
			glGetProgramInfoLog(programObject, info_len, NULL, info_log);
			Log("Error linking program:\n");
			Log(info_log);
			free(info_log);
		}
		glDeleteProgram(programObject);
		return 0;
	}

	// Delete these here because they are attached to the program object.
	for (int i = 0; i<2; i++)
		glDeleteShader(compiledShader[i]);

	// attributes
	return programObject;
}

struct Input
{
	Input()
	{
		memset(mInputs, -1, sizeof(int) * 8);
	}
	int mInputs[8];
};

struct Evaluation
{
	RenderTarget mTarget;
	unsigned int mShader;
	Input mInput;
	bool mbDirty;
};

std::vector<Evaluation> mEvaluations;
std::vector<int> mEvaluationOrderList;
int mDirtyCount = 0;

std::string mBaseShader;
FullScreenTriangle mFSQuad;
static const char* samplerName[] = { "Sampler0", "Sampler1", "Sampler2", "Sampler3", "Sampler4", "Sampler5", "Sampler6", "Sampler7" };
unsigned int equiRectTexture;

std::map<std::string, std::string> mGLSLs;


std::string ReplaceAll(std::string str, const std::string& from, const std::string& to)
{
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
	return str;
}


void SetEvaluationGLSL(const std::vector<std::string>& filenames)
{
	for (auto& filename : filenames)
	{
		std::ifstream t(filename);
		if (t.good())
		{
			std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());
			mGLSLs[filename] = str;
		}
	}

	mBaseShader = mGLSLs["Shader.glsl"];
	for (auto& filename : filenames)
	{
		std::string include = std::string("#include \"") + filename + "\"";
		mBaseShader = ReplaceAll(mBaseShader, include, mGLSLs[filename]);
	}
}

std::string GetEvaluationGLSL(const std::string& filename)
{
	return mGLSLs[filename];
}

void InitEvaluation()
{
	mFSQuad.Init();
}

unsigned int AddEvaluationTarget()
{
	Evaluation evaluation;
	evaluation.mTarget.initBuffer(256, 256, false);
	evaluation.mbDirty = true;
	mDirtyCount++;
	mEvaluations.push_back(evaluation);
	return mEvaluations.size() - 1;
}

void DelEvaluationTarget(int target)
{
	SetTargetDirty(target);
	Evaluation& ev = mEvaluations[target];
	glDeleteProgram(ev.mShader);
	ev.mTarget.destroy();
	mEvaluations.erase(mEvaluations.begin() + target);

	// shift all connections
	for (auto& evaluation : mEvaluations)
	{
		for (auto& inp : evaluation.mInput.mInputs)
		{
			if (inp >= target)
				inp--;
		}
	}
}

unsigned int GetEvaluationTexture(int target)
{
	return mEvaluations[target].mTarget.mGLTexID;
}



void SetEvaluationCall(int target, const std::string& shaderCall)
{
	std::string shd = mBaseShader;
	shd = ReplaceAll(shd, std::string("__FUNCTION__"), shaderCall);
	unsigned int& program = mEvaluations[target].mShader;
	if (program)
		glDeleteProgram(program);
	program = LoadShader(shd, "shd");
	SetTargetDirty(target);
}

void RunEvaluation()
{
	if (mEvaluationOrderList.empty())
		return;
	if (!mDirtyCount)
		return;

	
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		const Evaluation& evaluation = mEvaluations[mEvaluationOrderList[i]];
		if (!evaluation.mbDirty)
			continue;

		const Input& input = evaluation.mInput;
		const RenderTarget &tg = evaluation.mTarget;
		tg.bindAsTarget();
		glUseProgram(evaluation.mShader);
		int samplerIndex = 0;
		for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(evaluation.mShader, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			samplerIndex++;
			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex == -1)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				continue;
			}
			//assert(!mEvaluations[targetIndex].mbDirty);
			glBindTexture(GL_TEXTURE_2D, mEvaluations[targetIndex].mTarget.mGLTexID);
			TexParam(GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D);


		}
		//
		unsigned int parameter = glGetUniformLocation(evaluation.mShader, "equiRectEnvSampler");
		if (parameter != 0xFFFFFFFF)
		{
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_2D, equiRectTexture);
		}
		mFSQuad.Render();
	}

	for (auto& evaluation : mEvaluations)
	{
		if (evaluation.mbDirty)
		{
			evaluation.mbDirty = false;
			mDirtyCount--;
		}
	}
	assert(mDirtyCount == 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void AddEvaluationInput(int target, int slot, int source)
{
	mEvaluations[target].mInput.mInputs[slot] = source;
	SetTargetDirty(target);
}

void DelEvaluationInput(int target, int slot)
{
	mEvaluations[target].mInput.mInputs[slot] = -1;
	SetTargetDirty(target);
}

void SetEvaluationOrder(const std::vector<int> nodeOrderList)
{
	mEvaluationOrderList = nodeOrderList;
}

void SetTargetDirty(int target)
{
	if (!mEvaluations[target].mbDirty)
	{
		mDirtyCount++;
		mEvaluations[target].mbDirty = true;
	}
	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		if (mEvaluationOrderList[i] != target)
			continue;
		
		for (i++; i < mEvaluationOrderList.size(); i++)
		{
			Evaluation& currentEvaluation = mEvaluations[mEvaluationOrderList[i]];
			if (currentEvaluation.mbDirty)
				continue;

			for (auto inp : currentEvaluation.mInput.mInputs)
			{
				if (inp >= 0 && mEvaluations[inp].mbDirty)
				{
					mDirtyCount++;
					currentEvaluation.mbDirty = true;
					break; // at least 1 dirty
				}
			}
		}
	}
}

struct TransientTarget
{
	RenderTarget mTarget;
	int mUseCount;
};
std::vector<TransientTarget*> mFreeTargets;
int mTransientTextureMaxCount;
TransientTarget* GetTransientTarget(int width, int height, int useCount)
{
	if (mFreeTargets.empty())
	{
		TransientTarget *res = new TransientTarget;
		res->mTarget.initBuffer(width, height, false);
		res->mUseCount = useCount;
		mTransientTextureMaxCount++;
		return res;
	}
	TransientTarget *res = mFreeTargets.back();
	res->mUseCount = useCount;
	mFreeTargets.pop_back();
	return res;
}

void LoseTransientTarget(TransientTarget *transientTarget)
{
	transientTarget->mUseCount--;
	if (transientTarget->mUseCount <= 0)
		mFreeTargets.push_back(transientTarget);
}

void Bake(const char *szFilename, int target, int width, int height)
{
	if (mEvaluationOrderList.empty())
		return;

	if (target < 0 || target >= (int)mEvaluations.size())
		return;

	mTransientTextureMaxCount = 0;
	std::vector<int> evaluationUseCount(mEvaluationOrderList.size(), 0); // use count of texture by others
	std::vector<TransientTarget*> evaluationTransientTargets(mEvaluationOrderList.size(), NULL);
	for (auto& evaluation : mEvaluations)
	{
		for (int targetIndex : evaluation.mInput.mInputs)
			if (targetIndex != -1)
				evaluationUseCount[targetIndex]++;
	}
	if (evaluationUseCount[target] < 1)
		evaluationUseCount[target] = 1;

	// todo : revert pass. dec use count for parent nodes whose children have 0 use count

	for (size_t i = 0; i < mEvaluationOrderList.size(); i++)
	{
		size_t nodeIndex = mEvaluationOrderList[i];
		if (!evaluationUseCount[nodeIndex])
			continue;
		const Evaluation& evaluation = mEvaluations[nodeIndex];

		const Input& input = evaluation.mInput;

		TransientTarget* transientTarget = GetTransientTarget(width, height, evaluationUseCount[nodeIndex]);
		evaluationTransientTargets[nodeIndex] = transientTarget;
		transientTarget->mTarget.bindAsTarget();
		glUseProgram(evaluation.mShader);
		int samplerIndex = 0;
		for (size_t inputIndex = 0; inputIndex < 8; inputIndex++)
		{
			unsigned int parameter = glGetUniformLocation(evaluation.mShader, samplerName[inputIndex]);
			if (parameter == 0xFFFFFFFF)
				continue;
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			samplerIndex++;
			int targetIndex = input.mInputs[inputIndex];
			if (targetIndex == -1)
			{
				glBindTexture(GL_TEXTURE_2D, 0);
				continue;
			}

			glBindTexture(GL_TEXTURE_2D, evaluationTransientTargets[targetIndex]->mTarget.mGLTexID);
			LoseTransientTarget(evaluationTransientTargets[targetIndex]);
			TexParam(GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D);
		}
		//
		unsigned int parameter = glGetUniformLocation(evaluation.mShader, "equiRectEnvSampler");
		if (parameter != 0xFFFFFFFF)
		{
			glUniform1i(parameter, samplerIndex);
			glActiveTexture(GL_TEXTURE0 + samplerIndex);
			glBindTexture(GL_TEXTURE_2D, equiRectTexture);
		}
		mFSQuad.Render();
		if (nodeIndex == target)
			break;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glUseProgram(0);

	// save
	unsigned char *imgBits = new unsigned char[width * height * 4];
	glBindTexture(GL_TEXTURE_2D, evaluationTransientTargets[target]->mTarget.mGLTexID);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, imgBits);
	stbi_write_png(szFilename, width, height, 4, imgBits, width * 4);
	delete[] imgBits;
	
	// free transient textures
	for (auto transientTarget : mFreeTargets)
	{
		assert(transientTarget->mUseCount <= 0);
		transientTarget->mTarget.destroy();
	}

	Log("Texture %s saved. Using %d textures.\n", szFilename, mTransientTextureMaxCount);
}

void LoadEquiRectHDREnvLight(const std::string& filepath)
{
	HDRLoaderResult result;
	bool ret = HDRLoader::load(filepath.c_str(), result);
	if (!ret)
		return;
	glGenTextures(1, &equiRectTexture);
	glBindTexture(GL_TEXTURE_2D, equiRectTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, result.width, result.height, 0, GL_RGB, GL_FLOAT, result.cols);
	TexParam(GL_LINEAR, GL_LINEAR, GL_REPEAT, GL_REPEAT, GL_TEXTURE_2D);
}
