#include <SDL.h>
#include <GL/glew.h>
#include <SDL_opengl.h>
#include <gl/glu.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <cstdlib>
#include <openvr.h>
#include "lodepng.h"
#include "Matrices.h"
#include "pathtools.h"
#include "LoadShader.h"
#include "fractal_creator.h"

#if defined(POSIX)
#include "unistd.h"
#endif

using std::vector;

void ThreadSleep(unsigned long nMilliseconds)
{
#if defined(_WIN32)
	::Sleep(nMilliseconds);
#elif defined(POSIX)
	usleep(nMilliseconds * 1000);
#endif
}

class CGLRenderModel
{
public:
	CGLRenderModel(const std::string & sRenderModelName);
	~CGLRenderModel();

	bool BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture);
	void Cleanup();
	void Draw();
	const std::string & GetName() const { return m_sModelName; }



private:
	GLuint m_glVertBuffer;
	GLuint m_glIndexBuffer;
	GLuint m_glVertArray;
	GLuint m_glTexture;
	GLsizei m_unVertexCount;
	std::string m_sModelName;
};

static bool g_bPrintf = true;

//-----------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
class CMainApplication
{
public:
	CMainApplication(int argc, char *argv[]);
	virtual ~CMainApplication();

	bool BInit();
	bool BInitGL();
	bool BInitCompositor();

	void SetupRenderModels();

	void Shutdown();

	void RunMainLoop();
	bool HandleInput();
	void ProcessVREvent(const vr::VREvent_t & event);
	void RenderFrame();

	bool SetupTexturemaps();

	float defaultCameraY = 20;

	/*vector <float> vertdataarray = {
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
	};*/
	//-----------------------------------------------------------------------------
	// Create the initial scene geometry.
	//-----------------------------------------------------------------------------
	void SetupScene()
	{
		if (!m_pHMD)
			return;

		vr::HmdQuad_t chap_Rect;
		int xDivisions = 12, yDivisions = 12;

		m_pChaperone->GetPlayAreaRect(&chap_Rect);

		//Top left, top right, bottom right, bottom left (Clockwise).
		Vector3 tl = *new Vector3(chap_Rect.vCorners[0].v[0], chap_Rect.vCorners[0].v[1], chap_Rect.vCorners[0].v[2]),
			tr = *new Vector3(chap_Rect.vCorners[1].v[0], chap_Rect.vCorners[1].v[1], chap_Rect.vCorners[1].v[2]),
			br = *new Vector3(chap_Rect.vCorners[2].v[0], chap_Rect.vCorners[2].v[1], chap_Rect.vCorners[2].v[2]),
			bl = *new Vector3(chap_Rect.vCorners[3].v[0], chap_Rect.vCorners[3].v[1], chap_Rect.vCorners[3].v[2]);

		float width = abs(tr.x - bl.x) * 10;
		float height = abs(tr.z - bl.z) * 10;

		AddTerrain();
		MoveScenePos(*new Vector3(0, defaultCameraY, 0));

		m_uiVertcount = vertdataarray.size() / 3;

		//Position: Create a vertex buffer object (VBO), which basically moves the array[] to the GPU Memory
		glGenBuffers(1, &m_glSceneVertBuffer);
		//Bind VBO, makes it the current buffer in OpenGL's state machine
		glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
		//Tells GL that the GL_ARRAY_BUFFER is the size of the vector * the size of a float, and "gives it the address of the first value"
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), vertdataarray.data(), GL_STATIC_DRAW);

		//Colour: Create a vertex buffer object (VBO), which basically moves the array[] to the GPU Memory
		glGenBuffers(1, &m_glSceneColorBuffer);
		//Bind VBO, makes it the current buffer in OpenGL's state machine
		glBindBuffer(GL_ARRAY_BUFFER, m_glSceneColorBuffer);
		//Tells GL that the GL_ARRAY_BUFFER is the size of the vector * the size of a float, and "gives it the address of the first value"
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * colordataarray.size(), colordataarray.data(), GL_STATIC_DRAW);

		//vertex attribute object (VAO) remembers all of the vertex buffers (VBO's) 
		// that you want to use, and the memory layout of each one. 
		glGenVertexArrays(1, &m_unSceneVAO);
		//"bind it, to bring it in to focus in the state machine."
		glBindVertexArray(m_unSceneVAO);

		//Enable the first attribute, 0, Position.
		glEnableVertexAttribArray(0);
		//Bind GL state to the VBO
		glBindBuffer(GL_ARRAY_BUFFER, m_glSceneVertBuffer);
		//Defining layout of our first VBO within the VAO: ""0" means define the layout for attribute number 0. 
		//"3" means that the variables are vec3 made from every 3 floats (GL_FLOAT) in the buffer."
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		//Enable the Second attribute, 1, Color.
		glEnableVertexAttribArray(1);
		//Bind GL state to the VBO
		glBindBuffer(GL_ARRAY_BUFFER, m_glSceneColorBuffer);
		//Defining layout of our first VBO within the VAO: ""1" means define the layout for attribute number 1. 
		//"3" means that the variables are vec3 made from every 3 floats (GL_FLOAT) in the buffer."
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		//Unbind VAO and disable VBO. Is this necessary?
		//glBindVertexArray(0);
		//glDisableVertexAttribArray(0);
	}

	void AddPlane(int xDivisions, int yDivisions, int width, int height) {
		for (int x = 0; x < xDivisions; x++) {
			for (int y = 0; y < yDivisions; y++) {
				AddQuad(*new Vector3((width / xDivisions * x) - (width / 2), 0, (height / yDivisions * (y + 1)) - (height / 2)),
					*new Vector3((width / xDivisions * (x + 1)) - (width / 2), 0, (height / yDivisions * (y + 1)) - (height / 2)),
					*new Vector3((width / xDivisions * (x + 1)) - (width / 2), 0, (height / yDivisions * y) - (height / 2)),
					*new Vector3((width / xDivisions * x) - (width / 2), 0, (height / yDivisions * y) - (height / 2)));

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);

				colordataarray.push_back(0.2);
				colordataarray.push_back(((float)rand() / (RAND_MAX)));
				colordataarray.push_back(0.2);
			}
		}
	}

	void AddQuad(Vector3 p1, Vector3 p2, Vector3 p3, Vector3 p4) {		
		vertdataarray.push_back(p1.x);
		vertdataarray.push_back(p1.y);
		vertdataarray.push_back(p1.z);

		vertdataarray.push_back(p2.x);
		vertdataarray.push_back(p2.y);
		vertdataarray.push_back(p2.z);

		vertdataarray.push_back(p3.x);
		vertdataarray.push_back(p3.y);
		vertdataarray.push_back(p3.z);

		vertdataarray.push_back(p1.x);
		vertdataarray.push_back(p1.y);
		vertdataarray.push_back(p1.z);

		vertdataarray.push_back(p4.x);
		vertdataarray.push_back(p4.y);
		vertdataarray.push_back(p4.z);

		vertdataarray.push_back(p3.x);
		vertdataarray.push_back(p3.y);
		vertdataarray.push_back(p3.z);
	}

	void AddTerrain(int width = 200, int height = 200) {
		float maxHeight = 25.0f;

		vector<vector<float>> heightFractal = fractal_creator::MakeFractal(width, height, 13, maxHeight, 0);
		vector<vector<float>> colourFractal = fractal_creator::MakeFractal(width, height, 200, 1.0f, 0);


		for (int y = 0; y < heightFractal[0].size() - 1; y++)
		{
			for (int x = 0; x < heightFractal.size() - 1; x++)
			{
				AddQuad(*new Vector3(x, heightFractal[x][y], y),
					*new Vector3(x + 1, heightFractal[x + 1][y], y),
					*new Vector3(x + 1, heightFractal[x + 1][y + 1], y + 1),
					*new Vector3(x, heightFractal[x][y + 1], y + 1));

				colordataarray.push_back(colourFractal[x][y] / 4);
				colordataarray.push_back(0.5 + colourFractal[x][y] / 2);
				colordataarray.push_back(colourFractal[x][y] / 4);

				colordataarray.push_back(colourFractal[x + 1][y] / 4);
				colordataarray.push_back(0.45 + colourFractal[x + 1][y] / 2);
				colordataarray.push_back(colourFractal[x + 1][y] / 4);

				colordataarray.push_back(colourFractal[x + 1][y + 1] / 4);
				colordataarray.push_back(0.45 + colourFractal[x + 1][y + 1] / 2);
				colordataarray.push_back(colourFractal[x + 1][y + 1] / 4);

				colordataarray.push_back(colourFractal[x][y] / 4);
				colordataarray.push_back(0.45 + colourFractal[x][y] / 2);
				colordataarray.push_back(colourFractal[x][y] / 4);

				colordataarray.push_back(colourFractal[x][y + 1] / 4);
				colordataarray.push_back(0.45 + colourFractal[x][y + 1] / 2);
				colordataarray.push_back(colourFractal[x][y + 1] / 4);

				colordataarray.push_back(colourFractal[x + 1][y + 1] / 4);
				colordataarray.push_back(0.45 + colourFractal[x + 1][y + 1] / 2);
				colordataarray.push_back(colourFractal[x + 1][y + 1] / 4);
			}
		}

	}

	void ResetPlaySpace() {
		SceneModelMatrix[12] = 0;
		SceneModelMatrix[13] = defaultCameraY;
		SceneModelMatrix[14] = 0;
	}

	void MoveScenePos(Vector3 delta) {
		SceneModelMatrix.translate(delta);
	}

	void SetScenePos(Vector3 position) {
		SceneModelMatrix[12] = position.x;
		SceneModelMatrix[13] = position.y;
		SceneModelMatrix[14] = position.z;
	}

	void DrawControllers();

	bool SetupStereoRenderTargets();
	void SetupDistortion();
	void SetupCameras();

	void RenderStereoTargets();
	void RenderDistortion();
	void RenderScene(vr::Hmd_Eye nEye);

	Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
	Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);
	Matrix4 GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye);
	void UpdateHMDMatrixPose();

	Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);

	bool CreateAllShaders();

	void SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex);
	CGLRenderModel *FindOrLoadRenderModel(const char *pchRenderModelName);

private:
	bool m_bDebugOpenGL;
	bool m_bVerbose;
	bool m_bPerf;
	bool m_bVblank;
	bool m_bGlFinishHack;

	vr::IVRSystem *m_pHMD;
	vr::IVRChaperone *m_pChaperone;
	vr::IVRRenderModels *m_pRenderModels;
	std::string m_strDriver;
	std::string m_strDisplay;
	vr::TrackedDevicePose_t m_rTrackedDevicePose[vr::k_unMaxTrackedDeviceCount];
	Matrix4 m_rmat4DevicePose[vr::k_unMaxTrackedDeviceCount];
	bool m_rbShowTrackedDevice[vr::k_unMaxTrackedDeviceCount];

private: // SDL bookkeeping
	SDL_Window *m_pWindow;
	uint32_t m_nWindowWidth;
	uint32_t m_nWindowHeight;

	SDL_GLContext m_pContext;

private: // OpenGL bookkeeping
	int m_iTrackedControllerCount;
	int m_iTrackedControllerCount_Last;
	int m_iValidPoseCount;
	int m_iValidPoseCount_Last;

	std::string m_strPoseClasses;                            // what classes we saw poses for this frame
	char m_rDevClassChar[vr::k_unMaxTrackedDeviceCount];   // for each device, a character representing its class

	int m_iSceneVolumeInit;                                  // if you want something other than the default 20x20x20

	float m_fNearClip;
	float m_fFarClip;

	GLuint m_iTexture;

	unsigned int m_uiVertcount;

	GLuint m_glSceneVertBuffer;
	GLuint m_glSceneColorBuffer;
	GLuint m_unSceneVAO;
	GLuint m_unLensVAO;
	GLuint m_glIDVertBuffer;
	GLuint m_glIDIndexBuffer;
	unsigned int m_uiIndexSize;

	GLuint m_glControllerVertBuffer;
	GLuint m_unControllerVAO;
	unsigned int m_uiControllerVertcount;

	Matrix4 m_mat4HMDPose;
	Matrix4 m_mat4eyePosLeft;
	Matrix4 m_mat4eyePosRight;

	Matrix4 m_mat4ProjectionCenter;
	Matrix4 m_mat4ProjectionLeft;
	Matrix4 m_mat4ProjectionRight;

	struct VertexDataLens
	{
		Vector2 position;
		Vector2 texCoordRed;
		Vector2 texCoordGreen;
		Vector2 texCoordBlue;
	};

	GLuint m_unSceneProgramID;
	GLuint m_unLensProgramID;
	GLuint m_unControllerTransformProgramID;
	GLuint m_unRenderModelProgramID;

	GLint m_nSceneViewMatrix;
	GLint m_nSceneModelMatrix; 

	Matrix4 SceneModelMatrix;	//The actual Matrix

	GLint m_nControllerViewMatrix;
	GLint m_nRenderModelViewMatrix;

	struct FramebufferDesc
	{
		GLuint m_nDepthBufferId;
		GLuint m_nRenderTextureId;
		GLuint m_nRenderFramebufferId;
		GLuint m_nResolveTextureId;
		GLuint m_nResolveFramebufferId;
	};
	FramebufferDesc leftEyeDesc;
	FramebufferDesc rightEyeDesc;

	bool CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc);

	uint32_t m_nRenderWidth;
	uint32_t m_nRenderHeight;

	std::vector< CGLRenderModel * > m_vecRenderModels;
	CGLRenderModel *m_rTrackedDeviceToRenderModel[vr::k_unMaxTrackedDeviceCount];

	vector<float> cubedataarray = {
		-1.0f,-1.0f,-1.0f, 	// triangle 1 : begin
		-1.0f,-1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f, 	// triangle 1 : end
		1.0f, 1.0f,-1.0f, 	// triangle 2 : begin
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f, 	// triangle 2 : end
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		-1.0f,-1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f,-1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f,-1.0f,
		1.0f,-1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f,-1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f,-1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f,-1.0f, 1.0f,
	};
	vector<float> vertdataarray = {};
	vector<float> colordataarray = {};

	bool temp = false;
};

//-----------------------------------------------------------------------------
// Purpose: Outputs a set of optional arguments to debugging output, using
//          the printf format setting specified in fmt*.
//-----------------------------------------------------------------------------
void dprintf(const char *fmt, ...)
{
	va_list args;
	char buffer[2048];

	va_start(args, fmt);
	vsprintf_s(buffer, fmt, args);
	va_end(args);

	if (g_bPrintf)
		printf("%s", buffer);

	OutputDebugStringA(buffer);
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CMainApplication::CMainApplication(int argc, char *argv[])
	: m_pWindow(NULL)
	, m_pContext(NULL)
	, m_nWindowWidth(1280)
	, m_nWindowHeight(720)
	, m_unSceneProgramID(0)
	, m_unLensProgramID(0)
	, m_unControllerTransformProgramID(0)
	, m_unRenderModelProgramID(0)
	, m_pHMD(NULL)
	, m_pRenderModels(NULL)
	, m_pChaperone(NULL)
	, m_bDebugOpenGL(false)
	, m_bVerbose(false)
	, m_bPerf(false)
	, m_bVblank(false)
	, m_bGlFinishHack(true)
	, m_glControllerVertBuffer(0)
	, m_unControllerVAO(0)
	, m_unLensVAO(0)
	, m_unSceneVAO(0)
	, m_nSceneViewMatrix(-1)
	, m_nSceneModelMatrix(-1)
	, m_nControllerViewMatrix(-1)
	, m_nRenderModelViewMatrix(-1)
	, m_iTrackedControllerCount(0)
	, m_iTrackedControllerCount_Last(-1)
	, m_iValidPoseCount(0)
	, m_iValidPoseCount_Last(-1)
	, m_iSceneVolumeInit(20)
	, m_strPoseClasses("")
{

	for (int i = 1; i < argc; i++)
	{
		if (!stricmp(argv[i], "-gldebug"))
		{
			m_bDebugOpenGL = true;
		}
		else if (!stricmp(argv[i], "-verbose"))
		{
			m_bVerbose = true;
		}
		else if (!stricmp(argv[i], "-novblank"))
		{
			m_bVblank = false;
		}
		else if (!stricmp(argv[i], "-noglfinishhack"))
		{
			m_bGlFinishHack = false;
		}
		else if (!stricmp(argv[i], "-noprintf"))
		{
			g_bPrintf = false;
		}
		else if (!stricmp(argv[i], "-cubevolume") && (argc > i + 1) && (*argv[i + 1] != '-'))
		{
			m_iSceneVolumeInit = atoi(argv[i + 1]);
			i++;
		}
	}
	// other initialization tasks are done in BInit
	memset(m_rDevClassChar, 0, sizeof(m_rDevClassChar));
};


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CMainApplication::~CMainApplication()
{
	// work is done in Shutdown
	dprintf("Shutdown");
}


//-----------------------------------------------------------------------------
// Purpose: Helper to get a string from a tracked device property and turn it
//			into a std::string
//-----------------------------------------------------------------------------
std::string GetTrackedDeviceString(vr::IVRSystem *pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError *peError = NULL)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, NULL, 0, peError);
	if (unRequiredBufferLen == 0)
		return "";

	char *pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;
	return sResult;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::BInit()
{
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0)
	{
		printf("%s - SDL could not initialize! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	// Loading the SteamVR Runtime
	vr::EVRInitError eError = vr::VRInitError_None;
	m_pHMD = vr::VR_Init(&eError, vr::VRApplication_Scene);

	//Setup Chaperone reference.
	m_pChaperone = vr::VRChaperone();

	if (eError != vr::VRInitError_None)
	{
		m_pHMD = NULL;
		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to init VR runtime: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
		return false;
	}


	m_pRenderModels = (vr::IVRRenderModels *)vr::VR_GetGenericInterface(vr::IVRRenderModels_Version, &eError);
	if (!m_pRenderModels)
	{
		m_pHMD = NULL;
		vr::VR_Shutdown();

		char buf[1024];
		sprintf_s(buf, sizeof(buf), "Unable to get render model interface: %s", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "VR_Init Failed", buf, NULL);
		return false;
	}

	int nWindowPosX = 700;
	int nWindowPosY = 100;
	m_nWindowWidth = 1280;
	m_nWindowHeight = 720;
	Uint32 unWindowFlags = SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN;

	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	//SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY );
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
	if (m_bDebugOpenGL)
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

	m_pWindow = SDL_CreateWindow("BuddyVR", nWindowPosX, nWindowPosY, m_nWindowWidth, m_nWindowHeight, unWindowFlags);
	if (m_pWindow == NULL)
	{
		printf("%s - Window could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	m_pContext = SDL_GL_CreateContext(m_pWindow);
	if (m_pContext == NULL)
	{
		printf("%s - OpenGL context could not be created! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}

	glewExperimental = GL_TRUE;
	GLenum nGlewError = glewInit();
	if (nGlewError != GLEW_OK)
	{
		printf("%s - Error initializing GLEW! %s\n", __FUNCTION__, glewGetErrorString(nGlewError));
		return false;
	}
	glGetError(); // to clear the error caused deep in GLEW

	if (SDL_GL_SetSwapInterval(m_bVblank ? 1 : 0) < 0)
	{
		printf("%s - Warning: Unable to set VSync! SDL Error: %s\n", __FUNCTION__, SDL_GetError());
		return false;
	}


	m_strDriver = "No Driver";
	m_strDisplay = "No Display";

	m_strDriver = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);
	m_strDisplay = GetTrackedDeviceString(m_pHMD, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);

	std::string strWindowTitle = "BuddyVR - " + m_strDriver + " " + m_strDisplay;
	SDL_SetWindowTitle(m_pWindow, strWindowTitle.c_str());

	m_fNearClip = 0.1f;
	m_fFarClip = 300.0f;

	m_iTexture = 0;
	m_uiVertcount = 0;

	// 		m_MillisecondsTimer.start(1, this);
	// 		m_SecondsTimer.start(1000, this);

	if (!BInitGL())
	{
		printf("%s - Unable to initialize OpenGL!\n", __FUNCTION__);
		return false;
	}

	if (!BInitCompositor())
	{
		printf("%s - Failed to initialize VR Compositor!\n", __FUNCTION__);
		return false;
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Outputs the string in message to debugging output.
//          All other parameters are ignored.
//          Does not return any meaningful value or reference.
//-----------------------------------------------------------------------------
void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const char* message, const void* userParam)
{
	dprintf("GL Error: %s\n", message);
}


//-----------------------------------------------------------------------------
// Purpose: Initialize OpenGL. Returns true if OpenGL has been successfully
//          initialized, false if shaders could not be created.
//          If failure occurred in a module other than shaders, the function
//          may return true or throw an error. 
//-----------------------------------------------------------------------------
bool CMainApplication::BInitGL()
{
	if (m_bDebugOpenGL)
	{
		glDebugMessageCallback((GLDEBUGPROC)DebugCallback, nullptr);
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	}

	if (!CreateAllShaders())
		return false;

	SetupScene();
	SetupCameras();
	SetupStereoRenderTargets();
	SetupDistortion();

	SetupRenderModels();

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Initialize Compositor. Returns true if the compositor was
//          successfully initialized, false otherwise.
//-----------------------------------------------------------------------------
bool CMainApplication::BInitCompositor()
{
	vr::EVRInitError peError = vr::VRInitError_None;

	if (!vr::VRCompositor())
	{
		printf("Compositor initialization failed. See log file for details\n");
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::Shutdown()
{
	if (m_pHMD)
	{
		vr::VR_Shutdown();
		m_pHMD = NULL;
	}

	for (std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++)
	{
		delete (*i);
	}
	m_vecRenderModels.clear();

	if (m_pContext)
	{
		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_FALSE);
		glDebugMessageCallback(nullptr, nullptr);
		glDeleteBuffers(1, &m_glSceneVertBuffer);
		glDeleteBuffers(1, &m_glSceneColorBuffer);

		glDeleteBuffers(1, &m_glIDVertBuffer);
		glDeleteBuffers(1, &m_glIDIndexBuffer);

		if (m_unSceneProgramID)
		{
			glDeleteProgram(m_unSceneProgramID);
		}
		if (m_unControllerTransformProgramID)
		{
			glDeleteProgram(m_unControllerTransformProgramID);
		}
		if (m_unRenderModelProgramID)
		{
			glDeleteProgram(m_unRenderModelProgramID);
		}
		if (m_unLensProgramID)
		{
			glDeleteProgram(m_unLensProgramID);
		}

		glDeleteRenderbuffers(1, &leftEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &leftEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &leftEyeDesc.m_nResolveFramebufferId);

		glDeleteRenderbuffers(1, &rightEyeDesc.m_nDepthBufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nRenderTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nRenderFramebufferId);
		glDeleteTextures(1, &rightEyeDesc.m_nResolveTextureId);
		glDeleteFramebuffers(1, &rightEyeDesc.m_nResolveFramebufferId);

		if (m_unLensVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unLensVAO);
		}
		if (m_unSceneVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unSceneVAO);
		}
		if (m_unControllerVAO != 0)
		{
			glDeleteVertexArrays(1, &m_unControllerVAO);
		}
	}

	if (m_pWindow)
	{
		SDL_DestroyWindow(m_pWindow);
		m_pWindow = NULL;
	}

	SDL_Quit();
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::HandleInput()
{
	SDL_Event sdlEvent;
	bool bRet = false;

	while (SDL_PollEvent(&sdlEvent) != 0)
	{
		if (sdlEvent.type == SDL_QUIT)
		{
			bRet = true;
		}
		else if (sdlEvent.type == SDL_KEYDOWN)
		{
			if (sdlEvent.key.keysym.sym == SDLK_ESCAPE
				|| sdlEvent.key.keysym.sym == SDLK_q)
			{
				bRet = true;
			}
		}
	}

	// Process SteamVR events
	vr::VREvent_t event;
	while (m_pHMD->PollNextEvent(&event, sizeof(event)))
	{
		ProcessVREvent(event);
	}

	// Process SteamVR controller state
	for (vr::TrackedDeviceIndex_t unDevice = 0; unDevice < vr::k_unMaxTrackedDeviceCount; unDevice++)
	{
		vr::VRControllerState_t state;
		if (m_pHMD->GetControllerState(unDevice, &state))
		{
			//Hide controller if anything is pressed (test).
			//m_rbShowTrackedDevice[ unDevice ] = ( state.ulButtonPressed == 0);

			if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_ApplicationMenu)) {
				//temp.k_EButton_ApplicationMenu = true;
			}

			if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Trigger)) {

				if (temp == false) {
					const Matrix4 &controllerMatrix = m_rmat4DevicePose[unDevice];
					Vector4 controllerDir = controllerMatrix * Vector4(0, 0, 1, 0);
					Vector4 controllerCenter = controllerMatrix * Vector4(0, 0, 0, 1);
					Vector4 HMDCenter = m_mat4HMDPose * Vector4(0, 0, 0, 1);

					//This is in playspace relative coordinates, not scene!
					//Where Y = 0:
					float xIntercept = controllerCenter.x - ((controllerCenter.y / controllerDir.y)*controllerDir.x);
					float zIntercept = controllerCenter.z - ((controllerCenter.y / controllerDir.y)*controllerDir.z);
					
					Vector3 teleLocation = *new Vector3(-(HMDCenter.x - xIntercept), 0, -(HMDCenter.z - zIntercept));
					MoveScenePos(teleLocation);
					
					temp = true;
				}
			}

			if (state.ulButtonPressed & vr::ButtonMaskFromId(vr::k_EButton_Grip)) {
				//temp.k_EButton_Grip = true;
				temp = false;
			}

			if (state.ulButtonTouched & vr::ButtonMaskFromId(vr::k_EButton_SteamVR_Touchpad)) {
				//temp.k_EButton_SteamVR_Touchpad = true;
			}
		}
	}
	return bRet;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RunMainLoop()
{
	bool bQuit = false;

	SDL_StartTextInput();
	SDL_ShowCursor(SDL_DISABLE);

	while (!bQuit)
	{
		bQuit = HandleInput();

		RenderFrame();
	}

	SDL_StopTextInput();
}


//-----------------------------------------------------------------------------
// Purpose: Processes a single VR event
//-----------------------------------------------------------------------------
void CMainApplication::ProcessVREvent(const vr::VREvent_t & event)
{
	switch (event.eventType)
	{
	case vr::VREvent_TrackedDeviceActivated:
	{
		SetupRenderModelForTrackedDevice(event.trackedDeviceIndex);
		dprintf("Device %u attached. Setting up render model.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceDeactivated:
	{
		dprintf("Device %u detached.\n", event.trackedDeviceIndex);
	}
	break;
	case vr::VREvent_TrackedDeviceUpdated:
	{
		dprintf("Device %u updated.\n", event.trackedDeviceIndex);
	}
	break;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderFrame()
{
	// for now as fast as possible
	if (m_pHMD)
	{
		DrawControllers();
		RenderStereoTargets();
		RenderDistortion();

		vr::Texture_t leftEyeTexture = { (void*)leftEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Left, &leftEyeTexture);
		vr::Texture_t rightEyeTexture = { (void*)rightEyeDesc.m_nResolveTextureId, vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::Eye_Right, &rightEyeTexture);
	}

	if (m_bVblank && m_bGlFinishHack)
	{
		//$ HACKHACK. From gpuview profiling, it looks like there is a bug where two renders and a present
		// happen right before and after the vsync causing all kinds of jittering issues. This glFinish()
		// appears to clear that up. Temporary fix while I try to get nvidia to investigate this problem.
		// 1/29/2014 mikesart
		glFinish();
	}

	// SwapWindow
	SDL_GL_SwapWindow(m_pWindow);

	// Clear
		// We want to make sure the glFinish waits for the entire present to complete, not just the submission
		// of the command. So, we do a clear here right here so the glFinish will wait fully for the swap.
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Flush and wait for swap.
	if (m_bVblank)
	{
		glFlush();
		glFinish();
	}

	// Spew out the controller and pose count whenever they change.
	if (m_iTrackedControllerCount != m_iTrackedControllerCount_Last || m_iValidPoseCount != m_iValidPoseCount_Last)
	{
		m_iValidPoseCount_Last = m_iValidPoseCount;
		m_iTrackedControllerCount_Last = m_iTrackedControllerCount;

		dprintf("PoseCount:%d(%s) Controllers:%d\n", m_iValidPoseCount, m_strPoseClasses.c_str(), m_iTrackedControllerCount);
	}

	UpdateHMDMatrixPose();
}

//-----------------------------------------------------------------------------
// Purpose: Creates all the shaders used by HelloVR SDL
//-----------------------------------------------------------------------------
bool CMainApplication::CreateAllShaders()
{
	m_unSceneProgramID = LoadShader::LoadShaders("scene.vert", "scene.frag");
	m_nSceneViewMatrix = glGetUniformLocation(m_unSceneProgramID, "viewMatrix");
	m_nSceneModelMatrix = glGetUniformLocation(m_unSceneProgramID, "modelMatrix");


	if (m_nSceneViewMatrix == -1)
	{
		dprintf("Unable to find view matrix uniform in scene shader\n");
		return false;
	}

	if (m_nSceneModelMatrix == -1)
	{
		dprintf("Unable to find model matrix uniform in scene shader\n");
		return false;
	}

	m_unControllerTransformProgramID = LoadShader::LoadShaders("controller.vert", "controller.frag");
	m_nControllerViewMatrix = glGetUniformLocation(m_unControllerTransformProgramID, "matrix");
	if (m_nControllerViewMatrix == -1)
	{
		dprintf("Unable to find matrix uniform in controller shader\n");
		return false;
	}

	m_unRenderModelProgramID = LoadShader::LoadShaders("render_model.vert", "render_model.frag");
	m_nRenderModelViewMatrix = glGetUniformLocation(m_unRenderModelProgramID, "matrix");
	if (m_nRenderModelViewMatrix == -1)
	{
		dprintf("Unable to find matrix uniform in render model shader\n");
		return false;
	}

	m_unLensProgramID = LoadShader::LoadShaders("distortion.vert", "distortion.frag");


	return m_unSceneProgramID != 0
		&& m_unControllerTransformProgramID != 0
		&& m_unRenderModelProgramID != 0
		&& m_unLensProgramID != 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CMainApplication::SetupTexturemaps()
{
	std::string sExecutableDirectory = Path_StripFilename(Path_GetExecutablePath());
	std::string strFullPath = Path_MakeAbsolute("../cube_texture.png", sExecutableDirectory);

	std::vector<unsigned char> imageRGBA;
	unsigned nImageWidth, nImageHeight;
	unsigned nError = lodepng::decode(imageRGBA, nImageWidth, nImageHeight, strFullPath.c_str());

	if (nError != 0)
		return false;

	glGenTextures(1, &m_iTexture);
	glBindTexture(GL_TEXTURE_2D, m_iTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, nImageWidth, nImageHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, &imageRGBA[0]);

	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

	glBindTexture(GL_TEXTURE_2D, 0);

	return (m_iTexture != 0);
}

//-----------------------------------------------------------------------------
// Purpose: Draw all of the controllers as X/Y/Z lines
//-----------------------------------------------------------------------------
void CMainApplication::DrawControllers()
{
	// don't draw controllers if somebody else has input focus
	if (m_pHMD->IsInputFocusCapturedByAnotherProcess())
		return;

	std::vector<float> vertdataarray;

	m_uiControllerVertcount = 0;
	m_iTrackedControllerCount = 0;

	for (vr::TrackedDeviceIndex_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; ++unTrackedDevice)
	{
		if (!m_pHMD->IsTrackedDeviceConnected(unTrackedDevice))
			continue;

		if (m_pHMD->GetTrackedDeviceClass(unTrackedDevice) != vr::TrackedDeviceClass_Controller)
			continue;

		m_iTrackedControllerCount += 1;

		if (!m_rTrackedDevicePose[unTrackedDevice].bPoseIsValid)
			continue;

		const Matrix4 &mat = m_rmat4DevicePose[unTrackedDevice];

		Vector4 center = mat * Vector4(0, 0, 0, 1);

		for (int i = 0; i < 3; ++i)
		{
			Vector3 color(0, 0, 0);
			Vector4 point(0, 0, 0, 1);
			point[i] += 0.05f;  // offset in X, Y, Z
			color[i] = 1.0;  // R, G, B
			point = mat * point;
			vertdataarray.push_back(center.x);
			vertdataarray.push_back(center.y);
			vertdataarray.push_back(center.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			vertdataarray.push_back(point.x);
			vertdataarray.push_back(point.y);
			vertdataarray.push_back(point.z);

			vertdataarray.push_back(color.x);
			vertdataarray.push_back(color.y);
			vertdataarray.push_back(color.z);

			m_uiControllerVertcount += 2;
		}

		Vector4 start = mat * Vector4(0, 0, -0.02f, 1);
		Vector4 end = mat * Vector4(0, 0, -39.f, 1);
		Vector3 color(.92f, .92f, .71f);

		vertdataarray.push_back(start.x); vertdataarray.push_back(start.y); vertdataarray.push_back(start.z);
		vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);

		vertdataarray.push_back(end.x); vertdataarray.push_back(end.y); vertdataarray.push_back(end.z);
		vertdataarray.push_back(color.x); vertdataarray.push_back(color.y); vertdataarray.push_back(color.z);
		m_uiControllerVertcount += 2;
	}

	// Setup the VAO the first time through.
	if (m_unControllerVAO == 0)
	{
		glGenVertexArrays(1, &m_unControllerVAO);
		glBindVertexArray(m_unControllerVAO);

		glGenBuffers(1, &m_glControllerVertBuffer);
		glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

		GLuint stride = 2 * 3 * sizeof(float);
		GLuint offset = 0;

		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

		offset += sizeof(Vector3);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (const void *)offset);

		glBindVertexArray(0);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_glControllerVertBuffer);

	// set vertex data if we have some
	if (vertdataarray.size() > 0)
	{
		//$ TODO: Use glBufferSubData for this...
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertdataarray.size(), &vertdataarray[0], GL_STREAM_DRAW);
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupCameras()
{
	m_mat4ProjectionLeft = GetHMDMatrixProjectionEye(vr::Eye_Left);
	m_mat4ProjectionRight = GetHMDMatrixProjectionEye(vr::Eye_Right);
	m_mat4eyePosLeft = GetHMDMatrixPoseEye(vr::Eye_Left);
	m_mat4eyePosRight = GetHMDMatrixPoseEye(vr::Eye_Right);
}


//-----------------------------------------------------------------------------
// Purpose: Creates a frame buffer. Returns true if the buffer was set up.
//          Returns false if the setup failed.
//-----------------------------------------------------------------------------
bool CMainApplication::CreateFrameBuffer(int nWidth, int nHeight, FramebufferDesc &framebufferDesc)
{
	glGenFramebuffers(1, &framebufferDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nRenderFramebufferId);

	glGenRenderbuffers(1, &framebufferDesc.m_nDepthBufferId);
	glBindRenderbuffer(GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, nWidth, nHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, framebufferDesc.m_nDepthBufferId);

	glGenTextures(1, &framebufferDesc.m_nRenderTextureId);
	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId);
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, GL_RGBA8, nWidth, nHeight, true);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, framebufferDesc.m_nRenderTextureId, 0);

	glGenFramebuffers(1, &framebufferDesc.m_nResolveFramebufferId);
	glBindFramebuffer(GL_FRAMEBUFFER, framebufferDesc.m_nResolveFramebufferId);

	glGenTextures(1, &framebufferDesc.m_nResolveTextureId);
	glBindTexture(GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, nWidth, nHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferDesc.m_nResolveTextureId, 0);

	// check FBO status
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		return false;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------`
bool CMainApplication::SetupStereoRenderTargets()
{
	if (!m_pHMD)
		return false;

	m_pHMD->GetRecommendedRenderTargetSize(&m_nRenderWidth, &m_nRenderHeight);

	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, leftEyeDesc);
	CreateFrameBuffer(m_nRenderWidth, m_nRenderHeight, rightEyeDesc);

	return true;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::SetupDistortion()
{
	if (!m_pHMD)
		return;

	GLushort m_iLensGridSegmentCountH = 43;
	GLushort m_iLensGridSegmentCountV = 43;

	float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
	float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

	float u, v = 0;

	std::vector<VertexDataLens> vVerts(0);
	VertexDataLens vert;

	//left eye distortion verts
	float Xoffset = -1;
	for (int y = 0; y < m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x < m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = m_pHMD->ComputeDistortion(vr::Eye_Left, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	//right eye distortion verts
	Xoffset = 0;
	for (int y = 0; y < m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x < m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = m_pHMD->ComputeDistortion(vr::Eye_Right, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	for (GLushort y = 0; y < m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x < m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}
	m_uiIndexSize = vIndices.size();

	glGenVertexArrays(1, &m_unLensVAO);
	glBindVertexArray(m_unLensVAO);

	glGenBuffers(1, &m_glIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size()*sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size()*sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderStereoTargets()
{
	glClearColor(0.15f, 0.15f, 0.18f, 1.0f); // nice background color, but not black
	glEnable(GL_MULTISAMPLE);

	// Left Eye
	glBindFramebuffer(GL_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Left);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, leftEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, leftEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

	glEnable(GL_MULTISAMPLE);

	// Right Eye
	glBindFramebuffer(GL_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glViewport(0, 0, m_nRenderWidth, m_nRenderHeight);
	RenderScene(vr::Eye_Right);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_MULTISAMPLE);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, rightEyeDesc.m_nRenderFramebufferId);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, rightEyeDesc.m_nResolveFramebufferId);

	glBlitFramebuffer(0, 0, m_nRenderWidth, m_nRenderHeight, 0, 0, m_nRenderWidth, m_nRenderHeight,
		GL_COLOR_BUFFER_BIT,
		GL_LINEAR);

	glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
}


//-----------------------------------------------------------------------------
// Purpose: Renders a scene with respect to nEye.
//-----------------------------------------------------------------------------
void CMainApplication::RenderScene(vr::Hmd_Eye nEye)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);

	//Draw Scene
	glUseProgram(m_unSceneProgramID);
	glUniformMatrix4fv(m_nSceneViewMatrix, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
	glUniformMatrix4fv(m_nSceneModelMatrix, 1, GL_FALSE, SceneModelMatrix.get());
	glBindVertexArray(m_unSceneVAO);
	glDrawArrays(GL_TRIANGLES, 0, m_uiVertcount);
	glBindVertexArray(0);

	bool bIsInputCapturedByAnotherProcess = m_pHMD->IsInputFocusCapturedByAnotherProcess();

	if (!bIsInputCapturedByAnotherProcess)
	{
		// draw the controller axis lines
		glUseProgram(m_unControllerTransformProgramID);
		glUniformMatrix4fv(m_nControllerViewMatrix, 1, GL_FALSE, GetCurrentViewProjectionMatrix(nEye).get());
		glBindVertexArray(m_unControllerVAO);
		glDrawArrays(GL_LINES, 0, m_uiControllerVertcount);
		glBindVertexArray(0);
	}

	// ----- Render Model rendering - Drawing controllers -----
	glUseProgram(m_unRenderModelProgramID);

	for (uint32_t unTrackedDevice = 0; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_rTrackedDeviceToRenderModel[unTrackedDevice] || !m_rbShowTrackedDevice[unTrackedDevice])
			continue;

		const vr::TrackedDevicePose_t & pose = m_rTrackedDevicePose[unTrackedDevice];
		if (!pose.bPoseIsValid)
			continue;

		if (bIsInputCapturedByAnotherProcess && m_pHMD->GetTrackedDeviceClass(unTrackedDevice) == vr::TrackedDeviceClass_Controller)
			continue;

		const Matrix4 & matDeviceToTracking = m_rmat4DevicePose[unTrackedDevice];
		Matrix4 matMVP = GetCurrentViewProjectionMatrix(nEye) * matDeviceToTracking;
		glUniformMatrix4fv(m_nRenderModelViewMatrix, 1, GL_FALSE, matMVP.get());

		m_rTrackedDeviceToRenderModel[unTrackedDevice]->Draw();
	}

	glUseProgram(0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::RenderDistortion()
{
	glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, m_nWindowWidth, m_nWindowHeight);

	glBindVertexArray(m_unLensVAO);
	glUseProgram(m_unLensProgramID);

	//render left lens (first half of index array )
	glBindTexture(GL_TEXTURE_2D, leftEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	//render right lens (second half of index array )
	glBindTexture(GL_TEXTURE_2D, rightEyeDesc.m_nResolveTextureId);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Matrix Projection Eye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix44_t mat = m_pHMD->GetProjectionMatrix(nEye, m_fNearClip, m_fFarClip, vr::API_OpenGL);

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
		);
}


//-----------------------------------------------------------------------------
// Purpose: Gets an HMDMatrixPoseEye with respect to nEye.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!m_pHMD)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = m_pHMD->GetEyeToHeadTransform(nEye);
	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
		);

	return matrixObj.invert();
}


//-----------------------------------------------------------------------------
// Purpose: Gets a Current View Projection Matrix with respect to nEye,
//          which may be an Eye_Left or an Eye_Right.
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::GetCurrentViewProjectionMatrix(vr::Hmd_Eye nEye)
{
	Matrix4 matMVP;
	if (nEye == vr::Eye_Left)
	{
		matMVP = m_mat4ProjectionLeft * m_mat4eyePosLeft * m_mat4HMDPose;
	}
	else if (nEye == vr::Eye_Right)
	{
		matMVP = m_mat4ProjectionRight * m_mat4eyePosRight *  m_mat4HMDPose;
	}

	return matMVP;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMainApplication::UpdateHMDMatrixPose()
{
	if (!m_pHMD)
		return;

	vr::VRCompositor()->WaitGetPoses(m_rTrackedDevicePose, vr::k_unMaxTrackedDeviceCount, NULL, 0);

	m_iValidPoseCount = 0;
	m_strPoseClasses = "";
	for (int nDevice = 0; nDevice < vr::k_unMaxTrackedDeviceCount; ++nDevice)
	{
		if (m_rTrackedDevicePose[nDevice].bPoseIsValid)
		{
			m_iValidPoseCount++;
			m_rmat4DevicePose[nDevice] = ConvertSteamVRMatrixToMatrix4(m_rTrackedDevicePose[nDevice].mDeviceToAbsoluteTracking);
			if (m_rDevClassChar[nDevice] == 0)
			{
				switch (m_pHMD->GetTrackedDeviceClass(nDevice))
				{
				case vr::TrackedDeviceClass_Controller:        m_rDevClassChar[nDevice] = 'C'; break;
				case vr::TrackedDeviceClass_HMD:               m_rDevClassChar[nDevice] = 'H'; break;
				case vr::TrackedDeviceClass_Invalid:           m_rDevClassChar[nDevice] = 'I'; break;
				case vr::TrackedDeviceClass_Other:             m_rDevClassChar[nDevice] = 'O'; break;
				case vr::TrackedDeviceClass_TrackingReference: m_rDevClassChar[nDevice] = 'T'; break;
				default:                                       m_rDevClassChar[nDevice] = '?'; break;
				}
			}
			m_strPoseClasses += m_rDevClassChar[nDevice];
		}
	}

	if (m_rTrackedDevicePose[vr::k_unTrackedDeviceIndex_Hmd].bPoseIsValid)
	{
		m_mat4HMDPose = m_rmat4DevicePose[vr::k_unTrackedDeviceIndex_Hmd].invert();
	}
}


//-----------------------------------------------------------------------------
// Purpose: Finds a render model we've already loaded or loads a new one
//-----------------------------------------------------------------------------
CGLRenderModel *CMainApplication::FindOrLoadRenderModel(const char *pchRenderModelName)
{
	CGLRenderModel *pRenderModel = NULL;
	for (std::vector< CGLRenderModel * >::iterator i = m_vecRenderModels.begin(); i != m_vecRenderModels.end(); i++)
	{
		if (!stricmp((*i)->GetName().c_str(), pchRenderModelName))
		{
			pRenderModel = *i;
			break;
		}
	}

	// load the model if we didn't find one
	if (!pRenderModel)
	{
		vr::RenderModel_t *pModel;
		vr::EVRRenderModelError error;
		while (1)
		{
			error = vr::VRRenderModels()->LoadRenderModel_Async(pchRenderModelName, &pModel);
			if (error != vr::VRRenderModelError_Loading)
				break;

			ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render model %s - %s\n", pchRenderModelName, vr::VRRenderModels()->GetRenderModelErrorNameFromEnum(error));
			return NULL; // move on to the next tracked device
		}

		vr::RenderModel_TextureMap_t *pTexture;
		while (1)
		{
			error = vr::VRRenderModels()->LoadTexture_Async(pModel->diffuseTextureId, &pTexture);
			if (error != vr::VRRenderModelError_Loading)
				break;

			ThreadSleep(1);
		}

		if (error != vr::VRRenderModelError_None)
		{
			dprintf("Unable to load render texture id:%d for render model %s\n", pModel->diffuseTextureId, pchRenderModelName);
			vr::VRRenderModels()->FreeRenderModel(pModel);
			return NULL; // move on to the next tracked device
		}

		pRenderModel = new CGLRenderModel(pchRenderModelName);
		if (!pRenderModel->BInit(*pModel, *pTexture))
		{
			dprintf("Unable to create GL model from render model %s\n", pchRenderModelName);
			delete pRenderModel;
			pRenderModel = NULL;
		}
		else
		{
			m_vecRenderModels.push_back(pRenderModel);
		}
		vr::VRRenderModels()->FreeRenderModel(pModel);
		vr::VRRenderModels()->FreeTexture(pTexture);
	}
	return pRenderModel;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL a Render Model for a single tracked device
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModelForTrackedDevice(vr::TrackedDeviceIndex_t unTrackedDeviceIndex)
{
	if (unTrackedDeviceIndex >= vr::k_unMaxTrackedDeviceCount)
		return;

	// try to find a model we've already set up
	std::string sRenderModelName = GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_RenderModelName_String);
	CGLRenderModel *pRenderModel = FindOrLoadRenderModel(sRenderModelName.c_str());
	if (!pRenderModel)
	{
		std::string sTrackingSystemName = GetTrackedDeviceString(m_pHMD, unTrackedDeviceIndex, vr::Prop_TrackingSystemName_String);
		dprintf("Unable to load render model for tracked device %d (%s.%s)", unTrackedDeviceIndex, sTrackingSystemName.c_str(), sRenderModelName.c_str());
	}
	else
	{
		m_rTrackedDeviceToRenderModel[unTrackedDeviceIndex] = pRenderModel;
		m_rbShowTrackedDevice[unTrackedDeviceIndex] = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
void CMainApplication::SetupRenderModels()
{
	memset(m_rTrackedDeviceToRenderModel, 0, sizeof(m_rTrackedDeviceToRenderModel));

	if (!m_pHMD)
		return;

	for (uint32_t unTrackedDevice = vr::k_unTrackedDeviceIndex_Hmd + 1; unTrackedDevice < vr::k_unMaxTrackedDeviceCount; unTrackedDevice++)
	{
		if (!m_pHMD->IsTrackedDeviceConnected(unTrackedDevice))
			continue;

		SetupRenderModelForTrackedDevice(unTrackedDevice);
	}

}


//-----------------------------------------------------------------------------
// Purpose: Converts a SteamVR matrix to our local matrix class
//-----------------------------------------------------------------------------
Matrix4 CMainApplication::ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
		);
	return matrixObj;
}


//-----------------------------------------------------------------------------
// Purpose: Create/destroy GL Render Models
//-----------------------------------------------------------------------------
CGLRenderModel::CGLRenderModel(const std::string & sRenderModelName)
	: m_sModelName(sRenderModelName)
{
	m_glIndexBuffer = 0;
	m_glVertArray = 0;
	m_glVertBuffer = 0;
	m_glTexture = 0;
}


CGLRenderModel::~CGLRenderModel()
{
	Cleanup();
}


//-----------------------------------------------------------------------------
// Purpose: Allocates and populates the GL resources for a render model
//-----------------------------------------------------------------------------
bool CGLRenderModel::BInit(const vr::RenderModel_t & vrModel, const vr::RenderModel_TextureMap_t & vrDiffuseTexture)
{
	// create and bind a VAO to hold state for this model
	glGenVertexArrays(1, &m_glVertArray);
	glBindVertexArray(m_glVertArray);

	// Populate a vertex buffer
	glGenBuffers(1, &m_glVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vr::RenderModel_Vertex_t) * vrModel.unVertexCount, vrModel.rVertexData, GL_STATIC_DRAW);

	// Identify the components in the vertex buffer
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vPosition));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, vNormal));
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vr::RenderModel_Vertex_t), (void *)offsetof(vr::RenderModel_Vertex_t, rfTextureCoord));

	// Create and populate the index buffer
	glGenBuffers(1, &m_glIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint16_t) * vrModel.unTriangleCount * 3, vrModel.rIndexData, GL_STATIC_DRAW);

	glBindVertexArray(0);

	// create and populate the texture
	glGenTextures(1, &m_glTexture);
	glBindTexture(GL_TEXTURE_2D, m_glTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, vrDiffuseTexture.unWidth, vrDiffuseTexture.unHeight,
		0, GL_RGBA, GL_UNSIGNED_BYTE, vrDiffuseTexture.rubTextureMapData);

	// If this renders black ask McJohn what's wrong.
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat fLargest;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &fLargest);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, fLargest);

	glBindTexture(GL_TEXTURE_2D, 0);

	m_unVertexCount = vrModel.unTriangleCount * 3;

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the GL resources for a render model
//-----------------------------------------------------------------------------
void CGLRenderModel::Cleanup()
{
	if (m_glVertBuffer)
	{
		glDeleteBuffers(1, &m_glIndexBuffer);
		glDeleteVertexArrays(1, &m_glVertArray);
		glDeleteBuffers(1, &m_glVertBuffer);
		m_glIndexBuffer = 0;
		m_glVertArray = 0;
		m_glVertBuffer = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Draws an actual render model - Draws controllers in this.
//-----------------------------------------------------------------------------
void CGLRenderModel::Draw()
{
	glBindVertexArray(m_glVertArray);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_glTexture);

	glDrawElements(GL_TRIANGLES, m_unVertexCount, GL_UNSIGNED_SHORT, 0);

	glBindVertexArray(0);
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int main(int argc, char *argv[])
{
	CMainApplication *pMainApplication = new CMainApplication(argc, argv);

	if (!pMainApplication->BInit())
	{
		pMainApplication->Shutdown();
		return 1;
	}

	pMainApplication->RunMainLoop();

	pMainApplication->Shutdown();

	return 0;
}
