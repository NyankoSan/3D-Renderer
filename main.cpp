#include <iostream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "shader.h"
#include "vector3.h"

using namespace std;

#define DEFAULT_WINDOW_WIDTH 400
#define DEFAULT_WINDOW_HEIGHT 400

#define EPSILON 1e-5

#define MAX_BOUNCES 2

int windowWidth = DEFAULT_WINDOW_WIDTH;
int windowHeight = DEFAULT_WINDOW_HEIGHT;

float** pixelBuffer = new float*(nullptr);

class Ray {
public:
	Vector3 start;
	Vector3 dir;
};

class HitData {
public:
	double distance;
	Vector3 intersection, normal;
	bool isInside;
};

class SceneObject {
public:
	Vector3 ambient;
	Vector3 diffuse;
	Vector3 specular;
	double shininess;

	bool isGlass;
	double refractiveIndex = 1.003;

	virtual bool intersect(Ray ray, HitData& data) = 0;
	virtual ~SceneObject() = default;
};

class Sphere : public SceneObject {
public:
	Vector3 c_;
	double r_;

	Sphere(Vector3 c, double r) : c_(c), r_(r) {}
	Sphere() : c_(Vector3()), r_(1.0) {}

	bool intersect(Ray ray, HitData& data) override {
		double delta = pow(dot_prod(ray.dir, (ray.start - c_)), 2.0) - ((ray.start - c_).lengthsqr() - pow(r_, 2.0));
		double t;
		if (delta < 0) {
			return false;
		}
		else if (delta == 0) {
			t = -dot_prod(ray.dir, (ray.start - c_));
		}
		else {
			double t1 = -dot_prod(ray.dir, (ray.start - c_)) + pow(delta, 0.5);
			double t2 = -dot_prod(ray.dir, (ray.start - c_)) - pow(delta, 0.5);
			if (t2 < 0 && t1 < 0) {
				return false;
			}
			else if (t2 < 0) {
				t = t1;
			} 
			else {
				t = t2;
			}
		}
		data.intersection = ray.start + ray.dir * t;
		
		data.normal = data.intersection - c_;
		data.normal.normalize();
		
		if (dot_prod(ray.dir, data.normal) > 0) {
			data.normal = data.normal * -1.0;
			data.isInside = true;
		}
		else {
			data.isInside = false;
		}

		data.distance = t;
		return true;
	}
};

class Light {
public:
	Vector3 pos;
	Vector3 ambient;
	Vector3 diffuse;
	Vector3 specular;

	Light() : pos(Vector3()), ambient(Vector3()), diffuse(Vector3()), specular(Vector3()) {}

	Light(Vector3 pos, Vector3 ambient, Vector3 diffuse, Vector3 specular) :
		pos(pos), ambient(ambient), diffuse(diffuse), specular(specular) {}
};

class Camera {
public:
	Vector3 pos;
	Vector3 up;
	Vector3 lookAt;
	double focalLength;

	Camera() : pos(Vector3()), up(Vector3(0, 1, 0)), lookAt(Vector3(0, 0, 1)), focalLength(100) {}
	Camera(Vector3 p, Vector3 u, Vector3 l, double f) : pos(p), up(u), lookAt(l), focalLength(f) {}
};

class Scene {
public:
	vector<SceneObject*> sceneObjects;
	vector<Light> lights;
	vector<Camera> cameras;
	int activeCamera = 0;

	void moveCamera(int i, Vector3 pos) {
		cameras.at(i).pos = pos;
	}
	void addCamera(Camera camera) {
		cameras.push_back(camera);
	}
	void setActiveCamera(int i) {
		if (i < cameras.size()) {
			activeCamera = i;
		}
	}
	Camera getActiveCamera() const {
		return cameras.at(activeCamera);
	}
	void addLight(Light l) {
		lights.push_back(l);
	}
	void addObject(SceneObject* obj) {
		sceneObjects.push_back(obj);
	}

	~Scene() {
		cout << "WHY" << endl;
		for (SceneObject* ptr : sceneObjects) {
			delete ptr;
		}
	}
};

Scene scene[1];

void rayTrace(Ray ray, Vector3& rgb, const Scene& scene, int bounces = 0) {
	HitData hitData;
	HitData minHitData;
	double minDistance = DBL_MAX;
	SceneObject* closestObject = nullptr;

	for (SceneObject* obj : scene.sceneObjects) {
		if (!obj->intersect(ray, hitData)) {
			continue;
		}
		if (hitData.distance < EPSILON) {
			// Inside or behind object
			continue;
		}
		if (hitData.distance > minDistance) {
			// Blocked by another object
			continue;
		}
		minHitData = hitData;
		minDistance = hitData.distance;
		closestObject = obj;
	}

	if (closestObject == nullptr) {
		rgb.x_ = 0.7;
		rgb.y_ = 0.7;
		rgb.z_ = 1.0;
		return;
	}

	Vector3 V = ray.start - minHitData.intersection;
	V.normalize();
	Vector3 N = minHitData.normal;
	N.normalize();

	rgb.x_ = 0.0;
	rgb.y_ = 0.0;
	rgb.z_ = 0.0;

	for (Light l : scene.lights) {
		Vector3 L = l.pos - minHitData.intersection;
		L.normalize();

		Vector3 R = N * (dot_prod(L, N) * 2) - L;
		R.normalize();

		Vector3 ambient = elem_mult(closestObject->ambient, l.ambient);
		Vector3 diffuse = elem_mult(closestObject->diffuse, l.diffuse) * max(0.0, dot_prod(L, N));
		Vector3 specular = elem_mult(closestObject->specular, l.specular) * pow(max(0.0, dot_prod(R, V)), closestObject->shininess);

		rgb += ambient + diffuse + specular;
	}

	if (bounces == MAX_BOUNCES) {
		return;
	}
	
	if (!closestObject->isGlass) {
		return;
	}
	
	// REFLECTION
	Ray reflectionRay;
	reflectionRay.dir = N * (dot_prod(V, N) * 2) - V;
	reflectionRay.start = minHitData.intersection + minHitData.normal * EPSILON;

	Vector3 reflectionRGB;
	rayTrace(reflectionRay, reflectionRGB, scene, bounces + 1);
	
	// REFRACTION
	double n1 = minHitData.isInside ? closestObject->refractiveIndex : 1.0;
	double n2 = minHitData.isInside ? 1.0 : closestObject->refractiveIndex;
	double ratio = n1 / n2;

	double cos_i = dot_prod(minHitData.normal, V);
	double sine_t_sqr = pow(ratio, 2.0) * (1 - pow(cos_i, 2.0));
	double cos_t = sqrt(1 - sine_t_sqr);

	if (sine_t_sqr > 1) {
		// Total internal reflection
		rgb += elem_mult(reflectionRGB, closestObject->specular);
		return;
	}

	Vector3 transmissionV = V * -1.0 * ratio + minHitData.normal * (cos_i * ratio - sqrt(1 - sine_t_sqr));
	transmissionV.normalize();

	Ray refractionRay;
	refractionRay.start = minHitData.intersection - minHitData.normal * 1e-4;
	refractionRay.dir = transmissionV;
	
	Vector3 refractionRGB;
	rayTrace(refractionRay, refractionRGB, scene, bounces + 1);

	double R_perpendicular = pow((n1 * cos_i - n2 * cos_t) / (n1 * cos_i + n2 * cos_t), 2.0);
	double R_parallel = pow((n2 * cos_i - n1 * cos_t) / (n2 * cos_i + n1 * cos_t), 2.0);

	double reflectance = (R_perpendicular + R_parallel) / 2.0;
	double transmittance = 1 - reflectance;

	Vector3 I_r = reflectionRGB * reflectance;
	Vector3 I_t = refractionRGB * transmittance;

	rgb += elem_mult(closestObject->diffuse, (I_r + I_t));
}

void performRayTracing(const Scene& s) {
	Camera c = s.getActiveCamera();

	Vector3 cameraDir = c.lookAt - c.pos;
	cameraDir.normalize();
	
	Vector3 up = c.up;
	up.normalize();

	Vector3 right = cross_prod(cameraDir, up) * -1.0;
	right.normalize();
	
	Vector3 bottomLeftCorner =
		c.pos +
		cameraDir * c.focalLength -
		right * (0.5 * windowWidth) -
		up * (0.5 * windowHeight);

	int i, j;
	for (i = 0; i < windowWidth; i++) {
		for (j = 0; j < windowHeight; j++) {
			Ray ray;
			ray.start = c.pos;
			ray.dir = bottomLeftCorner + right * i + up * j - c.pos;
			ray.dir.normalize();

			Vector3 rgb;
			rayTrace(ray, rgb, s);

			float* pixel = &((*pixelBuffer)[i * 3 + j * windowWidth * 3]);

			pixel[0] = rgb.x_;
			pixel[1] = rgb.y_;
			pixel[2] = rgb.z_;
		}
	}
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	delete[] * pixelBuffer;

	*pixelBuffer = new float[width * height * 3];

	windowWidth = width;
	windowHeight = height;
	performRayTracing(scene[0]);
	glViewport(0, 0, windowWidth, windowHeight);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (action != GLFW_PRESS) {
		return;
	}

	switch (key) {
	case GLFW_KEY_W:
		((Sphere*)(scene[0].sceneObjects[3]))->c_ += Vector3(0, 50, 0);
		break;
	case GLFW_KEY_S:
		((Sphere*)(scene[0].sceneObjects[3]))->c_ += Vector3(0, -50, 0);
		break;
	case GLFW_KEY_D:
		((Sphere*)(scene[0].sceneObjects[3]))->c_ += Vector3(5, 0, 0);
		break;
	case GLFW_KEY_A:
		((Sphere*)(scene[0].sceneObjects[3]))->c_ += Vector3(-5, 0, 0);
		break;
	}
	performRayTracing(scene[0]);
}

int main() {
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, "OpenGL Window", NULL, NULL);
	
	if (!window) {
		std::cout << "Window failed to create" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "GLAD failed to load" << std::endl;
		glfwTerminate();
		return -1;
	}


	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);

	// SETUP

	float vertex_attributes[] = {
		-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
		1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		1.0f, 1.0f, 0.0f,		1.0f, 1.0f,
		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f,
	};

	int indices[] = {
		0, 1, 2,
		2, 3, 0
	};

	unsigned int VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_attributes), vertex_attributes, GL_STATIC_DRAW);


	unsigned int VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// Texture-Coords
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	unsigned int EBO;
	glGenBuffers(1, &EBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	Shader shader = Shader("VertexShader.glsl", "FragmentShader.glsl");

	scene[0].addCamera(Camera(
		Vector3(0,0, -500),
		Vector3(0, 1, 0),
		Vector3(0, 0, 0),
		500.0
	));
	scene[0].setActiveCamera(0);

	scene[0].addLight(Light(
		Vector3(0, 1000, 0),
		Vector3(0.0, 0.0, 0.0),
		Vector3(0.7, 0.7, 0.7),
		Vector3(0.3, 0.3, 0.3)
	));

	/*scene[0].addLight(Light(
		Vector3(0, 0, 0),
		Vector3(0.0, 0.0, 0.0),
		Vector3(0.7, 0.7, 0.7),
		Vector3(0.3, 0.3, 0.3)
	));*/

	SceneObject* objects[4];
	
	objects[0] = new Sphere(Vector3(-130, 80, 200), 100);
	objects[1] = new Sphere(Vector3(130, -80, 0), 100);
	objects[2] = new Sphere(Vector3(-130, -80, 0), 100);
	objects[3] = new Sphere(Vector3(0, -100, -200), 100);

	objects[0]->ambient = Vector3(0.0, 1.0, 0.0);
	objects[0]->diffuse = Vector3(0.7, 1.0, 0.8);
	objects[0]->specular = Vector3(1.0, 1.0, 1.0);
	objects[0]->shininess = 300;
	objects[0]->isGlass = true;

	objects[1]->ambient = Vector3(1.0, 1.0, 1.0);
	objects[1]->diffuse = Vector3(1.0, 1.0, 1.0);
	objects[1]->specular = Vector3(1.0, 1.0, 1.0);
	objects[1]->shininess = 0;
	objects[1]->isGlass = false;

	objects[2]->ambient = Vector3(1.0, 1.0, 1.0);
	objects[2]->diffuse = Vector3(1.0, 1.0, 1.0);
	objects[2]->specular = Vector3(1.0, 1.0, 1.0);
	objects[2]->shininess = 39;
	objects[2]->isGlass = false;

	objects[3]->ambient = Vector3(0.0, 0.0, 0.0);
	objects[3]->diffuse = Vector3(1.0, 0.0, 0.0);
	objects[3]->specular = Vector3(1.0, 1.0, 1.0);
	objects[3]->shininess = 500;
	objects[3]->refractiveIndex = 1.61;
	objects[3]->isGlass = true;


	scene[0].addObject(objects[0]);
	scene[0].addObject(objects[1]);
	scene[0].addObject(objects[2]);
	scene[0].addObject(objects[3]);

	framebuffer_size_callback(window, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT);

	int time = 0;
	while (!glfwWindowShouldClose(window))
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, windowWidth, windowHeight, 0, GL_RGB, GL_FLOAT, *pixelBuffer);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture);
		shader.use();
		shader.setInt("ourTexture", 0);
		glBindVertexArray(VAO);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwPollEvents();
		glfwSwapBuffers(window);
	}

	glfwTerminate();
	return 0;
}