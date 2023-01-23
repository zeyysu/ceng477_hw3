#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <GL/glew.h>   // The GL Header File
#include <OpenGL/gl.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

using namespace std;

int width = 640, height = 600;
int row, column;
int moves = 0, score = 0;
float objsize;
float minX = 1e6, maxX = -1e6;
float minY = 1e6, maxY = -1e6;
float minZ = 1e6, maxZ = -1e6;
GLuint gProgram[3];
string objfile;
GLuint gVertexAttribBuffer, gIndexBuffer;
GLint gInVertexLoc, gInNormalLoc;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;
bool disableInput = false;
bool playerChose = false;
struct Vertex
{
    Vertex(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Bunny{
    float scale;
    bool exploding;
    float xCoord;
    float yCoord;
    int colorId;
};

struct Texture
{
    Texture(GLfloat inU, GLfloat inV) : u(inU), v(inV) { }
    GLfloat u, v;
};

struct Normal
{
    Normal(GLfloat inX, GLfloat inY, GLfloat inZ) : x(inX), y(inY), z(inZ) { }
    GLfloat x, y, z;
};

struct Face
{
	Face(int v[], int t[], int n[]) {
		vIndex[0] = v[0];
		vIndex[1] = v[1];
		vIndex[2] = v[2];
		tIndex[0] = t[0];
		tIndex[1] = t[1];
		tIndex[2] = t[2];
		nIndex[0] = n[0];
		nIndex[1] = n[1];
		nIndex[2] = n[2];
	}
    GLuint vIndex[3], tIndex[3], nIndex[3];
};

struct Character {
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing;  // Offset from baseline to left/top of glyph
    GLuint Advance;    // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
vector<Vertex> gVertices;
vector<Texture> gTextures;
vector<Normal> gNormals;
vector<Face> gFaces;
vector<vector<Bunny> > bunnies;
glm::vec3 colors[] = {glm::vec3(0.5,0.5,0.0),glm::vec3(0.5,0.0,0.5),
glm::vec3(0.0,0.5,0.5), glm::vec3(0.5,0.0,0.0), glm::vec3(0.0,0.5,0.0), glm::vec3(0.0,0.0,0.5)};

void matchBunnies(){
    //first look at columnss
    int count = 0;
    int lastColor = -1;
    for(int i=0; i< column; i++){
        lastColor = -1;
        count = 0;
        for(int j=0; j<row; j++){
            if(bunnies[i][j].colorId == lastColor) count ++;
            else{
                if( count >= 3 ){
                    for(int k = j -count; k<j ; k++){
                        bunnies[i][k].exploding = true;
                        disableInput = true;
                    }
                }
                count = 1;
                lastColor = bunnies[i][j].colorId;
            }
        }
        if( count >= 3 ){
            for(int k = row - count; k<row ; k++){
                    bunnies[i][k].exploding = true;
                    disableInput = true;
            }
        }
    }
    //look at rows
    for(int j=0; j<row; j++){
        lastColor = -1;
        count = 0;
        for(int i=0; i< column; i++){
            if(bunnies[i][j].colorId == lastColor) count ++;
            else{
                if( count >= 3 ){
                    for(int k = i - count; k<i ; k++){
                        bunnies[k][j].exploding = true;
                        disableInput = true;
                    }
                }
                count = 1;
                lastColor = bunnies[i][j].colorId;
            }
        }
        if( count >= 3 ){
            for(int k = column - count; k<column ; k++){
                bunnies[k][j].exploding = true;
                disableInput = true;
            }
        }
    }    
}

void drawModel()
{
	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));

	glDrawElements(GL_TRIANGLES, gFaces.size() * 3, GL_UNSIGNED_INT, 0);
}

void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	static float angle = 0;

    if(!disableInput) matchBunnies();

    bool exploding = false;

    float scale = (17/objsize) / (max(column, row));
    glm::mat4 RT = glm::translate(glm::mat4(1.f), glm::vec3(-0.5*(maxX-minX)-minX,-0.5*(maxY-minY)-minY, -0.5*(maxZ-minZ)-minZ));
    glm::mat4 R = glm::rotate(glm::mat4(1.f), glm::radians(angle), glm::vec3(0, 1, 0));
    glm::mat4 T = glm::translate(glm::mat4(1.f), glm::vec3(-1.f*minX,-1.f*minY, -1.f));
    glm::mat4 ortMat = glm::ortho(-10.f,10.f,-10.f,10.f,-20.f,20.f);

    for(int i=0; i< column; i++){
        for(int j=0; j<row; j++){
            if(bunnies[i][j].exploding){
                if(bunnies[i][j].scale < 1.5){ 
                    bunnies[i][j].scale += 0.01;
                    disableInput = true;
                    exploding = true;
                }
                else {
                    if(!playerChose) score ++;
                    playerChose = false;
                    std::cout<<moves<<" "<<score<<endl;
                    Bunny *bunny = new Bunny();
                    bunny->exploding = false;
                    bunny->scale = 1.f;
                    bunny->xCoord = bunnies[i][j].xCoord;
                    bunny->yCoord = bunnies[i][row-1].yCoord +(18.f/row);
                    bunny->colorId = rand() % 6;
                    bunnies[i].push_back(*bunny); 
                    bunnies[i].erase(bunnies[i].begin()+j);
                    exploding = exploding | false;        
                }
            }
            if(bunnies[i][j].yCoord > (-8.f + (j+0.5)*(18.f/row))){
                bunnies[i][j].yCoord -= 0.05;
            }
            glm::mat4 S = glm::scale(glm::mat4(1.f), glm::vec3(scale*bunnies[i][j].scale, scale*bunnies[i][j].scale, scale*bunnies[i][j].scale));
            glm::mat4 T2 = glm::translate(glm::mat4(1.f), glm::vec3(bunnies[i][j].xCoord,bunnies[i][j].yCoord, -1.f));
            glm::mat4 modelMat = T2 * S  * R * RT;
            glm::mat4 modelMatInv = glm::transpose(glm::inverse(modelMat));
            glm::vec3 color = colors[bunnies[i][j].colorId];
            glm::vec3 lightPos = glm::vec3(bunnies[i][j].xCoord, bunnies[i][j].yCoord , 1.f);

            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMat"), 1, GL_FALSE, glm::value_ptr(modelMat));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "modelingMatInvTr"), 1, GL_FALSE, glm::value_ptr(modelMatInv));
            glUniformMatrix4fv(glGetUniformLocation(gProgram[0], "ortographicMat"), 1, GL_FALSE, glm::value_ptr(ortMat));
            glUniform3fv(glGetUniformLocation(gProgram[0], "lightPos"), 1, glm::value_ptr(lightPos));
            glUniform3fv(glGetUniformLocation(gProgram[0], "color"), 1, glm::value_ptr(color));
            drawModel();

        }
    }

    if(disableInput & !exploding){
        bool res = false;
        for(int i=0; i< column; i++){
            res = (bunnies[i][row-1].yCoord > (-8.f + (row-1+0.5)*(18.f/row)));
            if(res) break;
        }
        disableInput = res;
    }

	angle += 0.5;
}

bool ReadDataFromFile(
    const string& fileName, ///< [in]  Name of the shader file
    string&       data)     ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

void createVS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    glAttachShader(program, vs);
}

void createFS(GLuint& program, const string& filename)
{
    string shaderSource;

    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar* shader = (const GLchar*) shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    glAttachShader(program, fs);
}

void initShaders()
{
    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();


    createVS(gProgram[0], "vert.glsl");
    createFS(gProgram[0], "frag.glsl");
   
    // createVS(gProgram[1], "vert_text.glsl");
    // createFS(gProgram[1], "frag_text.glsl");


    // glBindAttribLocation(gProgram[1], 2, "vertex");

    glLinkProgram(gProgram[0]);
    glUseProgram(gProgram[0]);
}

bool ParseObj(const string& fileName)
{
    fstream myfile;

    // Open the input 
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            stringstream str(curLine);
            GLfloat c1, c2, c3;
            GLuint index[9];
            string tmp;

            if (curLine.length() >= 2)
            {
                if (curLine[0] == '#') // comment
                {
                    continue;
                }
                else if (curLine[0] == 'v')
                {
                    if (curLine[1] == 't') // texture
                    {
                        str >> tmp; // consume "vt"
                        str >> c1 >> c2;
                        gTextures.push_back(Texture(c1, c2));
                    }
                    else if (curLine[1] == 'n') // normal
                    {
                        str >> tmp; // consume "vn"
                        str >> c1 >> c2 >> c3;
                        gNormals.push_back(Normal(c1, c2, c3));
                    }
                    else // vertex
                    {
                        str >> tmp; // consume "v"
                        str >> c1 >> c2 >> c3;
                        gVertices.push_back(Vertex(c1, c2, c3));
                    }
                }
                else if (curLine[0] == 'f') // face
                {
                    str >> tmp; // consume "f"
					char c;
					int vIndex[3],  nIndex[3], tIndex[3];
					str >> vIndex[0]; str >> c >> c; // consume "//"
					str >> nIndex[0]; 
					str >> vIndex[1]; str >> c >> c; // consume "//"
					str >> nIndex[1]; 
					str >> vIndex[2]; str >> c >> c; // consume "//"
					str >> nIndex[2]; 

					assert(vIndex[0] == nIndex[0] &&
						   vIndex[1] == nIndex[1] &&
						   vIndex[2] == nIndex[2]); // a limitation for now

					// make indices start from 0
					for (int c = 0; c < 3; ++c)
					{
						vIndex[c] -= 1;
						nIndex[c] -= 1;
						tIndex[c] -= 1;
					}

                    gFaces.push_back(Face(vIndex, tIndex, nIndex));
                }
                else
                {
                    cout << "Ignoring unidentified line in obj file: " << curLine << endl;
                }
            }

            //data += curLine;
            if (!myfile.eof())
            {
                //data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

	/*
	for (int i = 0; i < gVertices.size(); ++i)
	{
		Vector3 n;

		for (int j = 0; j < gFaces.size(); ++j)
		{
			for (int k = 0; k < 3; ++k)
			{
				if (gFaces[j].vIndex[k] == i)
				{
					// face j contains vertex i
					Vector3 a(gVertices[gFaces[j].vIndex[0]].x, 
							  gVertices[gFaces[j].vIndex[0]].y,
							  gVertices[gFaces[j].vIndex[0]].z);

					Vector3 b(gVertices[gFaces[j].vIndex[1]].x, 
							  gVertices[gFaces[j].vIndex[1]].y,
							  gVertices[gFaces[j].vIndex[1]].z);

					Vector3 c(gVertices[gFaces[j].vIndex[2]].x, 
							  gVertices[gFaces[j].vIndex[2]].y,
							  gVertices[gFaces[j].vIndex[2]].z);

					Vector3 ab = b - a;
					Vector3 ac = c - a;
					Vector3 normalFromThisFace = (ab.cross(ac)).getNormalized();
					n += normalFromThisFace;
				}

			}
		}

		n.normalize();

		gNormals.push_back(Normal(n.x, n.y, n.z));
	}
	*/

	assert(gVertices.size() == gNormals.size());

    return true;
}

void initVBO()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao > 0);
    glBindVertexArray(vao);
    cout << "vao = " << vao << endl;

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	assert(glGetError() == GL_NONE);

	glGenBuffers(1, &gVertexAttribBuffer);
	glGenBuffers(1, &gIndexBuffer);

	assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

	glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

	gVertexDataSizeInBytes = gVertices.size() * 3 * sizeof(GLfloat);
	gNormalDataSizeInBytes = gNormals.size() * 3 * sizeof(GLfloat);
	int indexDataSizeInBytes = gFaces.size() * 3 * sizeof(GLuint);
	GLfloat* vertexData = new GLfloat [gVertices.size() * 3];
	GLfloat* normalData = new GLfloat [gNormals.size() * 3];
	GLuint* indexData = new GLuint [gFaces.size() * 3];

	for (int i = 0; i < gVertices.size(); ++i)
	{
		vertexData[3*i] = gVertices[i].x;
		vertexData[3*i+1] = gVertices[i].y;
		vertexData[3*i+2] = gVertices[i].z;

        minX = std::min(minX, gVertices[i].x);
        maxX = std::max(maxX, gVertices[i].x);
        minY = std::min(minY, gVertices[i].y);
        maxY = std::max(maxY, gVertices[i].y);
        minZ = std::min(minZ, gVertices[i].z);
        maxZ = std::max(maxZ, gVertices[i].z);
	}

    // std::cout << "minX = " << minX << std::endl;
    // std::cout << "maxX = " << maxX << std::endl;
    // std::cout << "minY = " << minY << std::endl;
    // std::cout << "maxY = " << maxY << std::endl;
    // std::cout << "minZ = " << minZ << std::endl;
    // std::cout << "maxZ = " << maxZ << std::endl;

    objsize = max(maxX-minX, maxY-minY);

	for (int i = 0; i < gNormals.size(); ++i)
	{
		normalData[3*i] = gNormals[i].x;
		normalData[3*i+1] = gNormals[i].y;
		normalData[3*i+2] = gNormals[i].z;
	}

	for (int i = 0; i < gFaces.size(); ++i)
	{
		indexData[3*i] = gFaces[i].vIndex[0];
		indexData[3*i+1] = gFaces[i].vIndex[1];
		indexData[3*i+2] = gFaces[i].vIndex[2];
	}


	glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexData);
	glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, normalData);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexDataSizeInBytes, indexData, GL_STATIC_DRAW);

	// done copying; can free now
	delete[] vertexData;
	delete[] normalData;
	delete[] indexData;

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void init() 
{
	ParseObj(objfile);

    glEnable(GL_DEPTH_TEST);
    initShaders();
    initVBO();
}



void reshape(GLFWwindow* window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    width = w;
    height = h;

    glViewport(0, 0, w, h);
}

void mainLoop(GLFWwindow* window)
{
    while (!glfwWindowShouldClose(window))
    {
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void initBunnies(){
    for(vector<Bunny> v : bunnies){
        v.clear();
    }
    bunnies.clear();
    srand(time(NULL));
    for(int i=0; i< column; i++){
        vector<Bunny> *b = new vector<Bunny>();
        for(int j=0;j<row;j++){
            Bunny *bunny = new Bunny();
            bunny->exploding = false;
            bunny->scale = 1.f;
            bunny->xCoord = -10.f + (i+0.5)*(20.f/column);
            bunny->yCoord = -8.f + (j+0.5)*(18.f/row);
            bunny->colorId = rand() % 6;
            b->push_back(*bunny);
        }
        bunnies.push_back(*b);
    }
}

void clicked( double xpos, double ypos){
    

    int x = (int)floor((xpos/width)*column);//get the grid coordinates of the click
    int y = row-1-(int)floor((ypos/(height-64))*row);//get the grid coordinates of the click
    if(x>=0 && x<column && y>=0 && y<row){
        bunnies[x][y].exploding = true;
        moves++;
        disableInput = true;
        playerChose = true;  
    }
}
void mousebutton(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        double xpos,ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
    }
    else if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE &&!disableInput){
        double xpos,ypos;
        glfwGetCursorPos(window, &xpos, &ypos);
        clicked(xpos,ypos);
    }
}

void keyboard(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
       initBunnies();
       moves = 0;
       score = 0;
       disableInput = false;
    }
}

int main(int argc, char** argv)   // Create Main Function For Bringing It All Together
{
    if(argc!=4){
        cout<<"usage: ./hw3 <grid_width> <grid_height> <object_file>";
        exit(1);
    }
    column = stoi(string(argv[1])); 
    row = stoi(string(argv[2])); 
    objfile = argv[3];

    initBunnies();

    GLFWwindow* window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
      glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
    window = glfwCreateWindow(width, height, "Simple Example", NULL, NULL);

    if (!window)
    {
        std::cout << "no window" << std::endl;
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

     if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    // char rendererInfo[512] = {0};
    // strcpy(rendererInfo, (const char*) glGetString(GL_RENDERER));
    // strcat(rendererInfo, " - ");
    // strcat(rendererInfo, (const char*) glGetString(GL_VERSION));
    char title[] = "CENG477 - THE3 - Rabbit Crush";
    glfwSetWindowTitle(window, title);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetMouseButtonCallback(window, mousebutton);

    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, width, height); // need to call this once ourselves
    mainLoop(window); // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}