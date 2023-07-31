#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <stb_image.h>
#include <stb_image_write.h>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

typedef uint8_t u8;

template <typename T>
static const T& min(const T& a, const T& b) { return a < b ? a : b; }

#define SIMPLIFIED_SRGB_FORMULA 0
// https://stackoverflow.com/questions/61138110/what-is-the-correct-gamma-correction-function
static float linearToGamma(float x) {
	#if SIMPLIFIED_SRGB_FORMULA
		return powf(x, 2.2f);
	#else
		return x <= 0.0031308f
			? x * 12.92f
			: powf(x, 1.0f / 2.4f) * 1.055f - 0.055f;
	#endif
};
static float gammaToLinear(float x) {
	#if SIMPLIFIED_SRGB_FORMULA
		return powf(x, 1.f/2.2f);
	#else
		return x <= 0.04045f
			? x / 12.92f
			: powf((x + 0.055f) / 1.055f, 2.4f);
	#endif
};

template <typename ReadConversionFn, typename WriteConversionFn>
static u8* boxFilter(const u8* inImg, int inW, int inH, ReadConversionFn&& readConversionFn, WriteConversionFn&& writeConversionFn)
{
	const int outW = inW / 2;
	const int outH = inH / 2;
	u8* outImg = new u8[outW * outH * 3];
	for (int y = 0; y < outH; y++)
	for (int x = 0; x < outW; x++) {
		float colorLinear[3] = { 0.f };
		for(int yy = 0; yy < 2; yy++)
		for(int xx = 0; xx < 2; xx++)
		for (int c = 0; c < 3; c++) {
			float col = inImg[3 * (inW * (2 * y + yy) + (2 * x + xx)) + c];
			col *= 1.f / 255.f;
			col = readConversionFn(col);
			colorLinear[c] += col;
		}

		for (int c = 0; c < 3; c++) {
			float col = 0.25f * colorLinear[c];
			col = writeConversionFn(col);
			outImg[3 * (outW * y + x) + c] = u8(min(roundf(255.f * col), 255.f));
		}
	}
	return outImg;
}

static int compareImages(const u8* imgA, const u8* imgB, int w, int h)
{
	int error = 0;
	for(int i = 0; i < w*h*3; i++) {
		const int d = imgA[i] - imgB[i];
		error += d * d;
	}
	return error;
}

GLuint makeTexture(const u8* img, int w, int h, bool gamma) {
	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	const GLenum internalFormat = gamma ? GL_SRGB8 : GL_RGB8;
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	return tex;
}

int main(int argc, char** argv)
{
	if(argc < 2) {
		puts("No image files provided\n");
		return -1;
	}

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow* window = glfwCreateWindow(100, 100, "test", NULL, NULL);
	if (!window) {
		puts("Error creating GLFW window\n");
		return -1;
	}
	glfwMakeContextCurrent(window);
	const int glVersion = gladLoadGL();
	if (glVersion == 0) {
		puts("Error initializing OpenGL\n");
		return -1;
	}
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	for (int i = 1; i < argc; i++) {
		int w, h, nc;
		u8* img = stbi_load(argv[i], &w, &h, &nc, 3);

		auto doNotConvert = [](float x) { return x; };
		u8* processedImg_cpu_no_conversion = boxFilter(img, w, h, doNotConvert, doNotConvert);
		const int saveStride = (w / 2) * 3;
		stbi_write_png("cpu_no_conversion.png", w / 2, h / 2, 3, processedImg_cpu_no_conversion, saveStride);

		u8* processedImg_cpu_conversion = boxFilter(img, w, h, gammaToLinear, linearToGamma);
		stbi_write_png("cpu_conversion.png", w / 2, h / 2, 3, processedImg_cpu_conversion, saveStride);

		GLuint tex_no_conversion = makeTexture(img, w, h, false);
		u8* processedImg_gpu_no_conversion = new u8[w * h * 3];
		glFinish();
		glGetTexImage(GL_TEXTURE_2D, 1, GL_RGB, GL_UNSIGNED_BYTE, processedImg_gpu_no_conversion);
		stbi_write_png("gpu_no_conversion.png", w / 2, h / 2, 3, processedImg_gpu_no_conversion, saveStride);

		GLuint tex_conversion = makeTexture(img, w, h, true);
		u8* processedImg_gpu_conversion = new u8[w * h * 3];
		glFinish();
		glGetTexImage(GL_TEXTURE_2D, 1, GL_RGB, GL_UNSIGNED_BYTE, processedImg_gpu_conversion);
		stbi_write_png("gpu_conversion.png", w / 2, h / 2, 3, processedImg_gpu_conversion, saveStride);

		const u8* imagesToCompare[] = { processedImg_cpu_no_conversion , processedImg_cpu_conversion, processedImg_gpu_no_conversion, processedImg_gpu_conversion };

		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				const int error = compareImages(imagesToCompare[i], imagesToCompare[j], w / 2, h / 2);
				printf("%12d", error);
			}
			printf("\n");
		}

		stbi_image_free(img);
	}
}