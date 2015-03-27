#ifndef TEXT2D_HPP
#define TEXT2D_HPP

class Text2D{
protected:
	unsigned int Text2DTextureID;
	unsigned int Text2DVertexBufferID;
	unsigned int Text2DUVBufferID;
	unsigned int Text2DShaderID;
	unsigned int Text2DUniformID;
	GLuint Text2DVertexLocation;
	GLuint Text2DUVLocation;

public:
	Text2D();
	~Text2D();
	void printText2D(const char * text, int x, int y, int size);
	void initText2D(const char * texturePath);
	void cleanupText2D();
};
#endif