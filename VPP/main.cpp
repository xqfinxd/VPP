#include "scene.hpp"

#define GLM_LEFT_HANDED 

static Drawable ball = Drawable("ball");
static Drawable broadSword = Drawable("broadSword");
static Drawable longSword = Drawable("longSword");

void draw_sample_1(Scene& scene)
{
	scene.EnableSkybox();
	scene.AddObject(&broadSword);
}

void draw_sample_2(Scene& scene)
{
	scene.AddShader("light");
	scene.UseShader("light");
	scene.AddObject(&ball);
}

void draw_sample_3(Scene& scene)
{
	scene.EnableSkybox();
	scene.AddShader("texture");
	scene.AddShader("light");
	ball.transform.scale = glm::vec3(0.5, 0.5, 0.5);
	broadSword.transform.position.z -= 0.2f;
	broadSword.transform.position.x += 0.2f;
	broadSword.transform.scale = glm::vec3(0.3, 0.3, 0.3);
	broadSword.name = "player";
	scene.UseShader("light");
	scene.AddObject(&ball);
	scene.UseShader("texture");
	scene.AddObject(&broadSword);
	scene.AddObject(&longSword);
}

int main(int argc, char** argv)
{
	Scene scene = Scene();
	draw_sample_1(scene);
	scene.Loop();
	return 0;
}