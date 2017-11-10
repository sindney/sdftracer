// links
// http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
// http://jamie-wong.com/2016/07/15/ray-marching-signed-distance-functions/
// http://www.iquilezles.org/www/articles/rmshadows/rmshadows.htm

#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <memory>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// image ops

struct Image {
	unsigned char* pData;
	int width, height, bpp;
};

struct Image* imgCreate(int w, int h, int bpp);

void imgSave(struct Image* pImg, const char* path);

void imgFree(struct Image* pImg);

//  sdf funcs

float sdWorld(const glm::vec3 &p);

glm::vec3 sdfNormal(const glm::vec3 &p);

float sdfTrace(const glm::vec3 &origin, const glm::vec3 &dir, float min, float max);

// const 

const int CANVAS_WIDTH = 512;

const int CANVAS_HEIGHT = 512;

const float NORMAL_EPSILON = 0.01f;

const float TRACE_EPSILON = 0.001f;

const float OBSTACLE_DIFFUSE = 0.8f;

const float OBSTACLE_SPECTULAR = 0.8f;

const float OBSTACLE_SPECTULAR_INTENSITY = 8.0f;

const glm::vec3 POINT_LIGHT_POS(100, 100, 220);

const float POINT_LIGHT_INTENSITY = 2;

const float POINT_LIGHT_START = 500;

const float POINT_LIGHT_END = 600;

const float SHADOW_PARAM = 1;

const glm::vec3 CAM_OFFSET(CANVAS_WIDTH / 2, 0, CANVAS_HEIGHT / 2);

int main(int argc, char *argv[])
{
	struct Image* pCanvas = NULL;

	pCanvas = imgCreate(CANVAS_WIDTH, CANVAS_HEIGHT, 1);
	if (pCanvas == NULL) 
		return 0;

	for (int x = 0; x < CANVAS_WIDTH; x++)
	{
		for (int y = 0; y < CANVAS_HEIGHT; y++)
		{
			unsigned char* pixel = &pCanvas->pData[x + y * CANVAS_WIDTH];

			glm::vec3 rayOrigin(x, 500.0f, y);
			glm::vec3 rayDir(0, -1, 0);

			float intensity = sdfTrace(rayOrigin - CAM_OFFSET, rayDir, 0.0f, 1000.0f);
			*pixel = std::min(1.0f, std::max(intensity, 0.0f)) * 255;
		}
	}

	imgSave(pCanvas, "result.png");
	imgFree(pCanvas);

	return 0;
}

float sdSphere(const glm::vec3 &p, float s)
{
	return glm::length(p) - s;
}

float sdBox(const glm::vec3 &p, const glm::vec3 &b)
{
	glm::vec3 d = glm::abs(p) - b;
	return glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) + glm::length(glm::max(d, 0.0f));
}

float sdTorus(const glm::vec3 &p, const glm::vec2 &t)
{
	glm::vec2 q(glm::length(glm::vec2(p.x, p.z)) - t.x, p.y);
	return glm::length(q) - t.y;
}

float sdPlane(const glm::vec3 &p, const glm::vec3 &n, float w)
{
	return glm::dot(p, n) + w;
}

glm::vec3 sdfNormal(const glm::vec3 &p)
{
	return glm::normalize(
		glm::vec3(
			sdWorld(glm::vec3(p.x + NORMAL_EPSILON, p.y, p.z)) - sdWorld(glm::vec3(p.x - NORMAL_EPSILON, p.y, p.z)), 
			sdWorld(glm::vec3(p.x, p.y + NORMAL_EPSILON, p.z)) - sdWorld(glm::vec3(p.x, p.y - NORMAL_EPSILON, p.z)), 
			sdWorld(glm::vec3(p.x, p.y, p.z + NORMAL_EPSILON)) - sdWorld(glm::vec3(p.x, p.y, p.z - NORMAL_EPSILON)) 
		)
	);
}

float sdWorld(const glm::vec3 &p)
{
	return std::min(
		std::min(
			sdSphere(p - glm::vec3(50, 0, -130), 60), 
			std::min(
				sdBox(p - glm::vec3(150, 0, 50), glm::vec3(40, 40, 40)), 
				sdTorus(p - glm::vec3(-100, 0, 60), glm::vec2(29, 30))
			)
		), 
		sdPlane(p, glm::vec3(0, 1, 0), 0)
	);
}

float sdfShadow(const glm::vec3 &origin, const glm::vec3 &dir, float min, float max)
{
	float res = 1.0f;
	for (float t = min; t < max;)
	{
		float d = sdWorld(origin + dir * t);
		if (d < TRACE_EPSILON)
			return 0.0f;
		res = std::min(res, SHADOW_PARAM * d / t);
		t += d;
	}
	return res;
}

float sdfTrace(const glm::vec3 &origin, const glm::vec3 &dir, float min, float max)
{
	for (float t = min; t < max;)
	{
		glm::vec3 worldPos = origin + dir * t;
		float d = sdWorld(worldPos);
		if (d < TRACE_EPSILON)
		{
			glm::vec3 v = -dir;
			glm::vec3 l = glm::normalize(POINT_LIGHT_POS - worldPos);
			glm::vec3 n = sdfNormal(worldPos);
			// glm::vec3 r = 2 * glm::dot(n, l) * n - l;
			glm::vec3 h = glm::normalize(l + v);

			float r = glm::length(POINT_LIGHT_POS - worldPos);

			float att = 1.0f;
			if (r <= POINT_LIGHT_START)
				att = 1.0f;
			else if(r >= POINT_LIGHT_END)
				att = 0.0f;
			else 
				att = (POINT_LIGHT_END - r) / (POINT_LIGHT_END - POINT_LIGHT_START);

			float shadow = sdfShadow(worldPos + n * TRACE_EPSILON, l, TRACE_EPSILON, r);

			return shadow * att * (
				POINT_LIGHT_INTENSITY * OBSTACLE_DIFFUSE * glm::dot(n, l) + 
				std::pow(OBSTACLE_SPECTULAR * glm::dot(n, h), OBSTACLE_SPECTULAR_INTENSITY));
		}
		t += d;
	}
	return 0.0f;
}

struct Image* imgCreate(int w, int h, int bpp)
{
	struct Image* img = (Image*)std::malloc(sizeof(struct Image));
	if (img == NULL) goto error;
	img->width = w;
	img->height = h;
	img->bpp = bpp;
	img->pData = (unsigned char *)std::malloc(w * h * bpp);
	if (img->pData == NULL) goto error;
	return img;

error:
	if (img != NULL) {
		free(img->pData);
		free(img);
	}
	return NULL;
}

void imgSave(struct Image* pImg, const char* path)
{
	stbi_write_png(path, pImg->width, pImg->height, pImg->bpp, pImg->pData, pImg->width * pImg->bpp);
}

void imgFree(struct Image* pImg)
{
	if (pImg == NULL) return;
	free(pImg->pData);
	free(pImg);
}