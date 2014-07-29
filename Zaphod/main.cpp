#include <SFML/Graphics.hpp>
#include "Rendering/Raytracer.h"
#include <ctime>
#include <string>

const int WIDTH = 1280;
const int HEIGHT = 720;

int main()
{
	//Initialize the window
    sf::RenderWindow window(sf::VideoMode(WIDTH, HEIGHT), "CPU Raytracer");

	//The array of pixels we want to fill (32 bit per pixel, 8 bit per channel RGBA)
	sf::Uint8* pixels = new sf::Uint8[WIDTH * HEIGHT * 4];

	//The texture we write the pixels into
	sf::Texture tex;
	tex.create(WIDTH, HEIGHT);

	//We need a sprite for rendering the texture with SFML
	sf::Sprite renderSprite;
	renderSprite.setTexture(tex);

	//Initialize the Raytrcer class with width, height and horizontal FOV
	Raytracer rt;
	rt.Initialize(WIDTH, HEIGHT, 90);

	//Update the pixel array
	rt.Render();

	time_t now = time(0);

	
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        window.clear();
		
		//Write the updated pixels to the texture
		tex.update(rt.GetPixels());

		//tex.copyToImage().saveToFile("Output" + std::string(ctime(&now)) + ".png");
		window.draw(renderSprite);
        window.display();
    }

	rt.Shutdown();

    return 0;
}