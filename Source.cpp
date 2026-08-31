#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <ctime>

using namespace std;

// -------------------------------------------- TRAFFIC LIGHT ---------------------------------------------------//
class TrafficLight
{
public:
	sf::RectangleShape lightShape;
	sf::Color color;
	int timer;
	string state;
	int rotate;

	// Constructor
	TrafficLight(float x, float y, string initialState) : timer(0), state(initialState)
	{
		lightShape.setSize(sf::Vector2f(8, 40)); // Traffic light size
		lightShape.setPosition(x, y);
		updateColor();
		rotate = 0;
	}

	// Getter
	sf::RectangleShape getLightShape()
	{
		return lightShape;
	}

	void rotateShape()
	{
		this->lightShape.rotate(90);
	}

	// Update the state of the traffic light
	void updateToGreen()
	{
		state = "Green";
		updateColor();
	}

	void updateToYellow()
	{
		state = "Yellow";
		updateColor();
	}

	void updateToRed()
	{
		state = "Red";
		updateColor();
	}

	// Update the color based on the current state
	void updateColor()
	{
		if (state == "Green")
		{
			color = sf::Color::Green;
		}
		else if (state == "Yellow")
		{
			color = sf::Color::Yellow;
		}
		else if (state == "Red")
		{
			color = sf::Color::Red;
		}
		lightShape.setFillColor(color);
	}

	void draw(sf::RenderWindow& window)
	{
		window.draw(lightShape);
	}

	std::string getState() const
	{
		return state;
	}
};



// -------------------------------------------- VEHICLE ---------------------------------------------------//
class Vehicle
{
public:
	int licenseNumber;
	bool challan;
	bool crossedTrafficLight;
	string type;
	float speed;
	float x, y;
	int rotate;
	int count = 0;

	// SFML
	sf::Sprite sprite;
	sf::Texture texture;

	// Constructor
	Vehicle(int license, float startX, float startY, const string& imageFile) : licenseNumber(license), x(startX), y(startY), speed(0), challan(false), rotate(0), crossedTrafficLight(false)
	{
		// Loading Image with thread-safe error handling
		if (!texture.loadFromFile(imageFile))
		{
			cerr << "Error: Could not load vehicle texture: " << imageFile << endl;
		}
		sprite.setTexture(texture);
		sprite.setPosition(x, y);
		sprite.setScale(0.4f, 0.4f);// 0.25,0.25
	}

	// Thread-safe getters and setters
	int getLicenseNumber() const { return licenseNumber; }
	float getSpeed() const { return speed; }
	bool getChallan() const { return challan; }
	string getType() const { return type; }
	float getX() const { return x; }
	float getY() const { return y; }

	void setLicenseNumber(int x) { licenseNumber = x; }
	void setSpeed(float x) { speed = x; }
	void setChallan(bool x) { challan = x; }
	void setType(const string& x) { type = x; }
	//void setX(float X) { x = X; }
	//void setY(float Y) { y = Y; }

	// Thread-safe move method
	void move(float offsetX, float offsetY)
	{
		//x = x + offsetX * speed;
		//y = y + offsetY * speed;
		x = x + offsetX;
		y = y + offsetY;
		sprite.setPosition(x, y);
	}

	// Thread-safe draw method
	void draw(sf::RenderWindow& window)
	{
		window.draw(sprite);
	}

	// Abstract Function
	virtual void updateSpeed() = 0;
};

class LightVehicle : public Vehicle
{
public:
	LightVehicle(int license, float startX, float startY, const string& imageFile)
		: Vehicle(license, startX, startY, imageFile)
	{
		// Seed randomness
		srand(static_cast<unsigned int>(time(0)));

		// Generate speed randomly between 1 and 60
		speed = static_cast<float>(rand() % 60 + 1);
		type = "light";
	}

	void updateSpeed() override
	{
		speed += 5;

		if (speed > 60 && count==0) // Challan condition for light vehicles
		{
			count++;
			//cout << "License No. " << this->getLicenseNumber() << " Speed Exceede: " << speed <<" km/h" << endl;
			challan = true;
		}
	}
};

class HeavyVehicle : public Vehicle
{
public:
	HeavyVehicle(int license, float startX, float startY, const string& imageFile)
		: Vehicle(license, startX, startY, imageFile)
	{
		// Seed randomness
		srand(static_cast<unsigned int>(time(0)));

		// Generate speed randomly between 1 and 40
		speed = static_cast<float>(rand() % 40 + 1);
		type = "heavy";
	}

	void updateSpeed() override
	{
		speed += 5;

		if (speed > 40 && count==0) // Challan condition for heavy vehicles
		{
			count++;
			challan = true;
		}
	}
};

class EmergencyVehicle : public Vehicle
{
public:
	EmergencyVehicle(int license, float startX, float startY, const string& imageFile)
		: Vehicle(license, startX, startY, imageFile)
	{
		// Seed randomness
		srand(static_cast<unsigned int>(time(0)));

		// Generate speed randomly between 1 and 80
		speed = static_cast<float>(rand() % 80 + 1);
		type = "emergency";
	}

	void updateSpeed() override
	{
		speed += 5;
	}
};



// -------------------------------------------- SIMULATION ---------------------------------------------------//
class Simulation
{
public:
	// Simulaiton Window
	sf::RenderWindow window;

	// Clock
	sf::Clock clock;

	// Background
	sf::Texture backgroundImage;
	sf::Sprite backgroundSprite;

	// vector to store Light vehicles and Emergency Vehicles
	vector<unique_ptr<Vehicle>> northVehicles;
	vector<unique_ptr<Vehicle>> southVehicles;
	vector<unique_ptr<Vehicle>> eastVehicles;
	vector<unique_ptr<Vehicle>> westVehicles;

	//vector to store Heavy Vehicles
	vector<unique_ptr<Vehicle>> northHeavyVehicles;
	vector<unique_ptr<Vehicle>> southHeavyVehicles;
	vector<unique_ptr<Vehicle>> eastHeavyVehicles;
	vector<unique_ptr<Vehicle>> westHeavyVehicles;

	// Traffic Lights
	TrafficLight southLight;
	TrafficLight northLight;
	TrafficLight eastLight;
	TrafficLight westLight;

	// traffic light count
	int southLightOccurance;
	int northLightOccurance;
	int westLightOccurance;
	int eastLightOccurance;
	int min;

	// Variables for simulation
	int simulationTime;
	bool Running;

	// Emergency Vehicle occurance
	bool northEmergency;
	bool eastEmergency;
	bool southEmergency;
	bool westEmergency;
	bool vehiclesUpdated;

	// Starting position for South vehicles
	float southStartX;
	float southStartY;

	// Starting position for North vehicles
	float northStartX;
	float northStartY;

	// Starting position for East vehicles
	float eastStartX;
	float eastStartY;

	// Starting position for West vehicles
	float westStartX;
	float westStartY;

	// Threads
	thread trafficLightThread;
	thread vehicleUpdateThread;

	thread spawnSouthLightVehicleThread;
	thread spawnSouthHeavyVehicleThread;
	thread spawnSouthEmergencyVehicleThread;

	thread spawnNorthLightVehicleThread;
	thread spawnNorthHeavyVehicleThread;
	thread spawnNorthEmergencyVehicleThread;

	thread spawnWestLightVehicleThread;
	thread spawnWestHeavyVehicleThread;
	thread spawnWestEmergencyVehicleThread;

	thread spawnEastLightVehicleThread;
	thread spawnEastHeavyVehicleThread;
	thread spawnEastEmergencyVehicleThread;

	// Mutex
	mutex emergencyMutex;

	// Constructor
	Simulation()
		: window(sf::VideoMode(700, 700), "Traffic Simulation"),
		southLight(394, 425, "Red"),// 394,425
		northLight(346, 268, "Green"),//346,268
		westLight(270, 353, "Red"),//270,353
		eastLight(422, 307, "Red"),//422,307
		simulationTime(100),// 60 seconds
		southStartX(352),// dont change its value from 352
		southStartY(650),// 480 MIN value
		northStartX(348),// dont change its value from 348
		northStartY(50),// 220 MAX value
		eastStartX(650),// 450 MIN value
		eastStartY(325),// dont change its value from 325
		westStartX(50),// 245 MAX value
		westStartY(350),// dont change its value from350
		northEmergency(false),
		eastEmergency(false),
		southEmergency(false),
		westEmergency(false),
		southLightOccurance(0),
		northLightOccurance(0),
		westLightOccurance(0),
		eastLightOccurance(0),
		min(1),
		vehiclesUpdated(false),
		Running(true)
	{
		// Load background image
		if (!backgroundImage.loadFromFile("assets/background4.png"))
		{
			cerr << "Error: Could not load background image!" << endl;
		}
		backgroundSprite.setTexture(backgroundImage);
		backgroundSprite.setScale(window.getSize().x / static_cast<float>(backgroundImage.getSize().x), window.getSize().y / static_cast<float>(backgroundImage.getSize().y));


		// Rotate North/South Road's Lights
		if (southLight.rotate == 0)
		{
			southLight.rotateShape();
			southLight.rotate = 1;
		}
		if (northLight.rotate == 0)
		{
			northLight.rotateShape();
			northLight.rotate = 1;
		}
	}

	// Destructor to ensure clean thread shutdown
	~Simulation()
	{
		Running = false;
		if (trafficLightThread.joinable()) trafficLightThread.join();
		if (vehicleUpdateThread.joinable()) vehicleUpdateThread.join();

		if (spawnSouthLightVehicleThread.joinable()) spawnSouthLightVehicleThread.join();
		if (spawnSouthHeavyVehicleThread.joinable()) spawnSouthHeavyVehicleThread.join();
		if (spawnSouthEmergencyVehicleThread.joinable()) spawnSouthEmergencyVehicleThread.join();

		if (spawnNorthLightVehicleThread.joinable()) spawnNorthLightVehicleThread.join();
		if (spawnNorthHeavyVehicleThread.joinable()) spawnNorthHeavyVehicleThread.join();
		if (spawnNorthEmergencyVehicleThread.joinable()) spawnNorthEmergencyVehicleThread.join();

		if (spawnWestLightVehicleThread.joinable()) spawnWestLightVehicleThread.join();
		if (spawnWestHeavyVehicleThread.joinable()) spawnWestHeavyVehicleThread.join();
		if (spawnWestEmergencyVehicleThread.joinable()) spawnWestEmergencyVehicleThread.join();

		if (spawnEastLightVehicleThread.joinable()) spawnEastLightVehicleThread.join();
		if (spawnEastHeavyVehicleThread.joinable()) spawnEastHeavyVehicleThread.join();
		if (spawnEastEmergencyVehicleThread.joinable()) spawnEastEmergencyVehicleThread.join();
	}

	void run()
	{
		// Start threads
		trafficLightThread = thread(&Simulation::updateTrafficLights, this);
		vehicleUpdateThread = thread(&Simulation::updateVehicles, this);

		spawnSouthLightVehicleThread = thread(&Simulation::spawnSouthLightVehicles, this);
		spawnSouthHeavyVehicleThread = thread(&Simulation::spawnSouthHeavyVehicles, this);
		spawnSouthEmergencyVehicleThread = thread(&Simulation::spawnSouthEmergencyVehicles, this);

		spawnNorthLightVehicleThread = thread(&Simulation::spawnNorthLightVehicles, this);
		spawnNorthHeavyVehicleThread = thread(&Simulation::spawnNorthHeavyVehicles, this);
		spawnNorthEmergencyVehicleThread = thread(&Simulation::spawnNorthEmergencyVehicles, this);

		spawnWestLightVehicleThread = thread(&Simulation::spawnWestLightVehicles, this);
		spawnWestHeavyVehicleThread = thread(&Simulation::spawnWestHeavyVehicles, this);
		spawnWestEmergencyVehicleThread = thread(&Simulation::spawnWestEmergencyVehicles, this);

		spawnEastLightVehicleThread = thread(&Simulation::spawnEastLightVehicles, this);
		spawnEastHeavyVehicleThread = thread(&Simulation::spawnEastHeavyVehicles, this);
		spawnEastEmergencyVehicleThread = thread(&Simulation::spawnEastEmergencyVehicles, this);

		while (window.isOpen() && simulationTime > clock.getElapsedTime().asSeconds())
		{
			handleEvents();
			render();
		}

		// Stop threads
		Running = false;
		trafficLightThread.join();
		vehicleUpdateThread.join();

		spawnSouthLightVehicleThread.join();
		spawnSouthHeavyVehicleThread.join();
		spawnSouthEmergencyVehicleThread.join();

		spawnNorthLightVehicleThread.join();
		spawnNorthHeavyVehicleThread.join();
		spawnNorthEmergencyVehicleThread.join();

		spawnWestLightVehicleThread.join();
		spawnWestHeavyVehicleThread.join();
		spawnWestEmergencyVehicleThread.join();

		spawnEastLightVehicleThread.join();
		spawnEastHeavyVehicleThread.join();
		spawnEastEmergencyVehicleThread.join();
	}


	void handleEvents()
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
			{
				window.close();
				Running = false;
			}
		}
	}

	void updateTrafficLights()
	{
		char nextToRun = 'n';
		int NE = 0;
		int WE = 0;
		int SE = 0;
		int EE = 0;
		while (Running)
		{
			if (northEmergency == false)
			{
				NE = 0;
			}
			if (westEmergency == false)
			{
				WE = 0;
			}
			if (eastEmergency == false)
			{
				EE = 0;
			}
			if (southEmergency == false)
			{
				SE = 0;
			}

			if (northEmergency)
			{
				this_thread::sleep_for(chrono::milliseconds(900));
				//lock_guard<mutex> lock(emergencyMutex);
				northLight.updateToGreen();
				southLight.updateToRed();
				eastLight.updateToRed();
				westLight.updateToRed();
				if (NE == 0)
				{
					cout << " Emergency Vehicle Coming from NORTH road " << endl;
					northLightOccurance++;
				}
				NE++;
				continue;
			}
			else if (eastEmergency)
			{
				this_thread::sleep_for(chrono::milliseconds(900));
				eastLight.updateToGreen();
				northLight.updateToRed();
				southLight.updateToRed();
				westLight.updateToRed();
				if (EE == 0)
				{
					cout << " Emergency Vehicle Coming from EAST road " << endl;
					eastLightOccurance++;
				}
				EE++;
				continue;
			}
			else if (southEmergency)
			{

				this_thread::sleep_for(chrono::milliseconds(900));
				southLight.updateToGreen();
				northLight.updateToRed();
				eastLight.updateToRed();
				westLight.updateToRed();
				if (SE == 0)
				{
					cout << " Emergency Vehicle Coming from SOUTH road " << endl;
					southLightOccurance++;
				}
				SE++;
				continue;
			}
			else if (westEmergency)
			{
				this_thread::sleep_for(chrono::milliseconds(900));
				//lock_guard<mutex> lock(emergencyMutex);
				westLight.updateToGreen();
				northLight.updateToRed();
				southLight.updateToRed();
				eastLight.updateToRed();
				if (WE == 0)
				{
					cout << " Emergency Vehicle Coming from WEST road " << endl;
					westLightOccurance++;
				}
				WE++;
				continue;
			}


			this_thread::sleep_for(chrono::seconds(4));

			int min = northLightOccurance;
			nextToRun = 'n';

			if (eastLightOccurance < min)
			{
				min = eastLightOccurance;
				nextToRun = 'e';
			}
			else if (southLightOccurance < min)
			{
				min = southLightOccurance;
				nextToRun = 's';
			}
			else if (westLightOccurance < min)
			{
				min = westLightOccurance;
				nextToRun = 'w';
			}

			switch (nextToRun)
			{
			case ('n'):
			{
				eastLight.updateToYellow();
				northLight.updateToRed();
				southLight.updateToRed();
				westLight.updateToRed();
				this_thread::sleep_for(chrono::seconds(1));
				eastLight.updateToGreen();
				northLightOccurance++;
				break;
			}

			case('e'):
			{
				southLight.updateToYellow();
				eastLight.updateToRed();
				northLight.updateToRed();
				westLight.updateToRed();
				this_thread::sleep_for(chrono::seconds(1));
				southLight.updateToGreen();
				eastLightOccurance++;
				break;
			}

			case('s'):
			{
				westLight.updateToYellow();
				southLight.updateToRed();
				northLight.updateToRed();
				eastLight.updateToRed();
				this_thread::sleep_for(chrono::seconds(1));
				westLight.updateToGreen();
				southLightOccurance++;
				break;
			}

			case('w'):
			{
				northLight.updateToYellow();
				westLight.updateToRed();
				eastLight.updateToRed();
				southLight.updateToRed();
				this_thread::sleep_for(chrono::seconds(1));
				northLight.updateToGreen();
				westLightOccurance++;
				break;
			}
			default:
				cout << "Error Occurred" << endl;
				break;
			}



		}
	}


	// SOUTH VEHICLE SPAWNING
	void spawnSouthLightVehicles()
	{
		while (Running)
		{


			int currentTime = static_cast<int>(clock.getElapsedTime().asSeconds());

			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int lightType = rand() % 3;
			if (lightType == 0)
			{
				southVehicles.push_back(make_unique<LightVehicle>(license, southStartX, southStartY, "assets/new/car.png"));
			}
			else if (lightType == 1)
			{
				southVehicles.push_back(make_unique<LightVehicle>(license, southStartX, southStartY, "assets/new/car2.png"));
			}
			else
			{
				southVehicles.push_back(make_unique<LightVehicle>(license, southStartX, southStartY, "assets/new/bike.png"));
			}
			simulationTime++;
			this_thread::sleep_for(chrono::seconds(8));
		}
	}

	void spawnSouthHeavyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));
			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 2;
			if (spawnVehicle == 0)
			{
				southHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, southStartX+20, southStartY, "assets/new/truck.png"));
			}
			else
			{
				southHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, southStartX+20, southStartY, "assets/new/bus.png"));
			}
			simulationTime++;
		}

	}

	void spawnSouthEmergencyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));
			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 20;
			if (spawnVehicle == 0)
			{
				southEmergency = true;
				int emergencyType = rand() % 3;
				if (emergencyType == 0)
				{
					southVehicles.push_back(make_unique<EmergencyVehicle>(license, southStartX, southStartY, "assets/new/ambulance.jpg"));
				}
				else if (emergencyType == 1)
				{
					southVehicles.push_back(make_unique<EmergencyVehicle>(license, southStartX, southStartY, "assets/new/police_car.png"));
				}
				else
				{
					southVehicles.push_back(make_unique<EmergencyVehicle>(license, southStartX, southStartY, "assets/new/fire_truck.png"));
				}
				simulationTime++;
			}
		}
	}


	// NORTH VEHICLE SPAWNING
	void spawnNorthLightVehicles()
	{
		while (Running)
		{


			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int lightType = rand() % 3;
			if (lightType == 0)
			{
				northVehicles.push_back(make_unique<LightVehicle>(license, northStartX, northStartY, "assets/new/car.png"));
			}
			else if (lightType == 1)
			{
				northVehicles.push_back(make_unique<LightVehicle>(license, northStartX, northStartY, "assets/new/bike.png"));
			}
			else
			{
				northVehicles.push_back(make_unique<LightVehicle>(license, northStartX, northStartY, "assets/new/car2.png"));
			}
			simulationTime++;

			// Rotate vehicles so they face down
			northVehicles.back().get()->sprite.rotate(180);
			this_thread::sleep_for(chrono::seconds(9));
		}
	}

	void spawnNorthHeavyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));
			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 2;
			if (spawnVehicle == 0)
			{
				northHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, northStartX-20, northStartY, "assets/new/truck.png"));
				northHeavyVehicles.back().get()->sprite.rotate(180);// 180 degrees
				simulationTime++;
			}
			else
			{
				northHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, northStartX-20, northStartY, "assets/new/bus.png"));
				northHeavyVehicles.back().get()->sprite.rotate(180);// 180 degrees
				simulationTime++;
			}
		}
	}

	void spawnNorthEmergencyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));

			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 10;
			if (spawnVehicle < 1)
			{
				northEmergency = true;
				int emergencyType = rand() % 3;
				if (emergencyType == 0)
				{
					northVehicles.push_back(make_unique<EmergencyVehicle>(license, northStartX, northStartY, "assets/new/ambulance.jpg"));
					// Rotate vehicles so they face right
					northVehicles.back().get()->sprite.rotate(180);
				}
				else if (emergencyType == 1)
				{
					northVehicles.push_back(make_unique<EmergencyVehicle>(license, northStartX, northStartY, "assets/new/police_car.png"));
					// Rotate vehicles so they face right
					northVehicles.back().get()->sprite.rotate(180);
				}
				else
				{
					northVehicles.push_back(make_unique<EmergencyVehicle>(license, northStartX, northStartY, "assets/new/fire_truck.png"));
					// Rotate vehicles so they face right
					northVehicles.back().get()->sprite.rotate(180);// 180 degrees
				}
				simulationTime++;
			}
		}
	}

	
	// EAST VEHICLE SPAWNING

	void spawnEastLightVehicles()
	{
		while (Running)
		{


			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int lightType = rand() % 2;
			if (lightType == 0)
			{
				eastVehicles.push_back(make_unique<LightVehicle>(license, eastStartX, eastStartY+20, "assets/new/car.png"));
			}
			else
			{
				eastVehicles.push_back(make_unique<LightVehicle>(license, eastStartX, eastStartY+20, "assets/new/bike.png"));
			}
			simulationTime++;

			// Rotate vehicles so they face left
			eastVehicles.back().get()->sprite.rotate(270);
			this_thread::sleep_for(chrono::seconds(8));
		}
	}

	void spawnEastHeavyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(25));
			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 2;
			if (spawnVehicle == 0)
			{
				eastHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, eastStartX, eastStartY, "assets/new/truck.png"));
				eastHeavyVehicles.back().get()->sprite.rotate(270);// 270 degrees
				simulationTime++;
			}
			else
			{
				eastHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, eastStartX, eastStartY, "assets/new/bus.png"));
				eastHeavyVehicles.back().get()->sprite.rotate(270);// 270 degrees
				simulationTime++;
			}
		}
	}

	void spawnEastEmergencyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));

			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 10;
			if (spawnVehicle == 0)
			{
				eastEmergency = true;
				int emergencyType = rand() % 3;
				if (emergencyType == 0)
				{
					eastVehicles.push_back(make_unique<EmergencyVehicle>(license, eastStartX, eastStartY+20, "assets/new/ambulance.jpg"));
					// Rotate vehicles so they face right
					eastVehicles.back().get()->sprite.rotate(270);
				}
				else if (emergencyType == 1)
				{
					eastVehicles.push_back(make_unique<EmergencyVehicle>(license, eastStartX, eastStartY+20, "assets/new/police_car.png"));
					// Rotate vehicles so they face right
					eastVehicles.back().get()->sprite.rotate(270);
				}
				else
				{
					eastVehicles.push_back(make_unique<EmergencyVehicle>(license, eastStartX, eastStartY+20, "assets/new/fire_truck.png"));
					// Rotate vehicles so they face right
					eastVehicles.back().get()->sprite.rotate(270);// 270 degrees
				}
				simulationTime++;
			}
		}
	}
	
	// WEST VEHICLE SPAWNING
	void spawnWestLightVehicles()
	{
		while (Running)
		{


			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int lightType = rand() % 2;
			if (lightType == 0)
			{
				westVehicles.push_back(make_unique<LightVehicle>(license, westStartX, westStartY, "assets/new/car.png"));
				simulationTime++;
			}
			else
			{
				westVehicles.push_back(make_unique<LightVehicle>(license, westStartX, westStartY, "assets/new/bike.png"));
				simulationTime++;
			}

			// Rotate vehicles so they face right
			westVehicles.back().get()->sprite.rotate(90);
			this_thread::sleep_for(chrono::seconds(8));
		}
	}

	void spawnWestHeavyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));
			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 2;
			if (spawnVehicle == 0)
			{
				westHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, westStartX, westStartY+20, "assets/new/truck.png"));
				westHeavyVehicles.back().get()->sprite.rotate(90);// 90 degrees
				simulationTime++;
			}
			else
			{
				westHeavyVehicles.push_back(make_unique<HeavyVehicle>(license, westStartX, westStartY+20, "assets/new/bus.png"));
				westHeavyVehicles.back().get()->sprite.rotate(90);// 90 degrees
				simulationTime++;
			}
		}
	}

	void spawnWestEmergencyVehicles()
	{
		while (Running)
		{
			this_thread::sleep_for(chrono::seconds(20));

			int license = simulationTime; // Use simulation time as a unique identifier for the vehicle

			int spawnVehicle = rand() % 10;
			if (spawnVehicle < 2)
			{
				westEmergency = true;
				int emergencyType = rand() % 3;
				if (emergencyType == 0)
				{
					westVehicles.push_back(make_unique<EmergencyVehicle>(license, westStartX, westStartY, "assets/new/ambulance.jpg"));
					// Rotate vehicles so they face right
					westVehicles.back().get()->sprite.rotate(90);// 90 degrees
				}
				else if (emergencyType == 1)
				{
					westVehicles.push_back(make_unique<EmergencyVehicle>(license, westStartX, westStartY, "assets/new/police_car.png"));
					// Rotate vehicles so they face right
					westVehicles.back().get()->sprite.rotate(90);// 90 degrees
				}
				else
				{
					westVehicles.push_back(make_unique<EmergencyVehicle>(license, westStartX, westStartY, "assets/new/fire_truck.png"));
					// Rotate vehicles so they face right
					westVehicles.back().get()->sprite.rotate(90);// 90 degrees
				}
				simulationTime++;
			}
		}
	}



	//------------------------- UPDATE VEHICLES --------------------------//

	void updateVehicles()
	{
		while (Running)
		{
			// Process SOUTH vehicles
			for (size_t i = 0; i < southVehicles.size(); i++)
			{
				auto& vehicle = southVehicles[i];

				// To prevent collision with vehicles in front
				if ((i != 0) && (southVehicles[i]->getY() < southVehicles[i - 1]->getY() + 60))
				{
					// Skip this iteration and dont update vehicle's position
					continue;
				}
				else
				{
					// If traffic light is not crossed yet
					if (!vehicle->crossedTrafficLight)
					{
						// if light is green then move
						if (southLight.getState() == "Green")
						{
							vehicle->move(0.0, -1.0);//-0.5

							if (vehicle->getY() < southLight.getLightShape().getPosition().y)
							{
								// exit from emergency state when emergency vehicle crosses traffic light
								if (vehicle->getType() == "emergency")
								{
									//lock_guard<mutex> lock(emergencyMutex);
									southEmergency = false;
								}
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((southLight.getState() == "Red" && (vehicle->getY() > (southLight.getLightShape().getPosition().y + 15)))
							|| (southLight.getState() == "Yellow" && (vehicle->getY() > (southLight.getLightShape().getPosition().y + 15))))
						{
							// if light is red or yellow but the vehicle has not reached to the traffic light yet
							vehicle->move(0.0, -1.0);//-0.5
						}
					}
					else
					{
						// if vehicle has already crossed the traffic light
						vehicle->move(0.0, -2.0);//-0.6
					}

					int temp = rand() % 10;
					if (temp == 0)
						vehicle->updateSpeed();
				}
			}

			// Process SOUTH HEAVY Vehicles
			for (size_t i = 0; i < southHeavyVehicles.size(); i++)
			{
				auto& vehicle = southHeavyVehicles[i];

				// To prevent collision with vehicles in front
				if ((i != 0) && (southHeavyVehicles[i]->getY() < southHeavyVehicles[i - 1]->getY() + 60))
				{
					// Skip this iteration and dont update vehicle's position
					continue;
				}
				else
				{
					// If traffic light is not crossed yet
					if (!vehicle->crossedTrafficLight)
					{
						// if light is green then move
						if (southLight.getState() == "Green")
						{
							vehicle->move(0.0, -1.0);//-0.5

							if (vehicle->getY() < southLight.getLightShape().getPosition().y)
							{
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((southLight.getState() == "Red" && (vehicle->getY() > (southLight.getLightShape().getPosition().y + 15)))
							|| (southLight.getState() == "Yellow" && (vehicle->getY() > (southLight.getLightShape().getPosition().y + 15))))
						{
							// if light is red but the vehicle has not reached to the traffic light yet
							vehicle->move(0.0, -1.0);//-0.5
						}
					}
					else
					{
						// if vehicle has already crossed the traffic light
						vehicle->move(0.0, -2.0);//-0.6
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process NORTH vehicles
			for (size_t i = 0; i < northVehicles.size(); i++)
			{
				auto& vehicle = northVehicles[i];

				if ((i != 0) && (northVehicles[i]->getY() > northVehicles[i - 1]->getY() - 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (northLight.getState() == "Green")
						{
							vehicle->move(0.0, 1.0);

							if (vehicle->getY() > northLight.getLightShape().getPosition().y)
							{
								// exit from emergency state when emergency vehicle crosses traffic light
								if (vehicle->getType() == "emergency")
								{
									//lock_guard<mutex> lock(emergencyMutex);
									northEmergency = false;
								}
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((northLight.getState() == "Red" && (vehicle->getY() < (northLight.getLightShape().getPosition().y - 10)))
							|| (northLight.getState() == "Yellow" && (vehicle->getY() < (northLight.getLightShape().getPosition().y - 10))))
						{
							vehicle->move(0.0, 1.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(0.0, 2.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process NORTH HEAVY Vehicles
			for (size_t i = 0; i < northHeavyVehicles.size(); i++)
			{
				auto& vehicle = northHeavyVehicles[i];

				if ((i != 0) && (northHeavyVehicles[i]->getY() > northHeavyVehicles[i - 1]->getY() - 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (northLight.getState() == "Green")
						{
							vehicle->move(0.0, 1.0);

							if (vehicle->getY() > northLight.getLightShape().getPosition().y)
							{
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((northLight.getState() == "Red" && (vehicle->getY() < (northLight.getLightShape().getPosition().y - 10)))
							|| (northLight.getState() == "Yellow" && (vehicle->getY() < (northLight.getLightShape().getPosition().y - 10))))
						{
							vehicle->move(0.0, 1.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(0.0, 2.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process WEST vehicles
			for (size_t i = 0; i < westVehicles.size(); i++)
			{
				auto& vehicle = westVehicles[i];

				if ((i != 0) && (westVehicles[i]->getX() > westVehicles[i - 1]->getX() - 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (westLight.getState() == "Green")
						{
							vehicle->move(1.0, 0.0);

							if (vehicle->getX() > westLight.getLightShape().getPosition().x)
							{
								// exit from emergency state when emergency vehicle crosses traffic light
								if (vehicle->getType() == "emergency")
								{
									//lock_guard<mutex> lock(emergencyMutex);
									westEmergency = false;
								}
								vehicle->crossedTrafficLight = true;
							}
						}
						else if ((westLight.getState() == "Red" && (vehicle->getX() < (westLight.getLightShape().getPosition().x - 10)))
							|| (westLight.getState() == "Yellow" && (vehicle->getX() < (westLight.getLightShape().getPosition().x - 10))))
						{
							vehicle->move(1.0, 0.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(2.0, 0.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process WEST HEAVY Vehicles
			for (size_t i = 0; i < westHeavyVehicles.size(); i++)
			{
				auto& vehicle = westHeavyVehicles[i];

				if ((i != 0) && (westHeavyVehicles[i]->getX() > westHeavyVehicles[i - 1]->getX() - 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (westLight.getState() == "Green")
						{
							vehicle->move(1.0, 0.0);

							if (vehicle->getX() > westLight.getLightShape().getPosition().x)
							{
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((westLight.getState() == "Red" && (vehicle->getX() < (westLight.getLightShape().getPosition().x - 10)))
							|| (westLight.getState() == "Yellow" && (vehicle->getX() < (westLight.getLightShape().getPosition().x - 10))))
						{
							vehicle->move(1.0, 0.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(2.0, 0.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process EAST vehicles
			for (size_t i = 0; i < eastVehicles.size(); i++)
			{
				auto& vehicle = eastVehicles[i];

				if ((i != 0) && (eastVehicles[i]->getX() < eastVehicles[i - 1]->getX() + 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (eastLight.getState() == "Green")
						{
							vehicle->move(-1.0, 0.0);

							if (vehicle->getX() < eastLight.getLightShape().getPosition().x)
							{
								// exit from emergency state when emergency vehicle crosses traffic light
								if (vehicle->getType() == "emergency")
								{
									//lock_guard<mutex> lock(emergencyMutex);
									eastEmergency = false;
								}
								vehicle->crossedTrafficLight = true;
							}
						}
						else if ((eastLight.getState() == "Red" && (vehicle->getX() > (eastLight.getLightShape().getPosition().x + 15))) ||
							(eastLight.getState() == "Yellow" && (vehicle->getX() > (eastLight.getLightShape().getPosition().x + 15))))
						{
							vehicle->move(-1.0, 0.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(-2.0, 0.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Process EAST HEAVY Vehicles
			for (size_t i = 0; i < eastHeavyVehicles.size(); i++)
			{
				auto& vehicle = eastHeavyVehicles[i];

				if ((i != 0) && (eastHeavyVehicles[i]->getX() < eastHeavyVehicles[i - 1]->getX() + 60))
				{
					continue;
				}
				else
				{
					if (!vehicle->crossedTrafficLight)
					{
						// Obey traffic light if not crossed yet
						if (eastLight.getState() == "Green")
						{
							vehicle->move(-1.0, 0.0);

							if (vehicle->getX() < eastLight.getLightShape().getPosition().x)
							{
								vehicle->crossedTrafficLight = true; // Mark as crossed
							}
						}
						else if ((eastLight.getState() == "Red" && (vehicle->getX() > (eastLight.getLightShape().getPosition().x + 15)))
							|| (eastLight.getState() == "Yellow" && (vehicle->getX() > (eastLight.getLightShape().getPosition().x + 15))))
						{
							vehicle->move(-1.0, 0.0);
						}
					}
					else
					{
						// Keep moving if already crossed
						vehicle->move(-2.0, 0.0);
					}
					int temp = rand() % 10;
					if (temp == 0)
					vehicle->updateSpeed();
				}
			}

			// Remove vehicles that have moved off-screen

			// Remove SOUTH vehicles
			for (size_t i = 0; i < southVehicles.size(); )
			{
				if (southVehicles[i]->getY() < 0)
				{
					southVehicles.erase(southVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove SOUTH HEAVY vehicles
			for (size_t i = 0; i < southHeavyVehicles.size(); )
			{
				if (southHeavyVehicles[i]->getY() < 0)
				{
					southHeavyVehicles.erase(southHeavyVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove NORTH vehicles
			for (size_t i = 0; i < northVehicles.size(); )
			{
				if (northVehicles[i]->getY() > 700)
				{
					northVehicles.erase(northVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove NORTH HEAVY vehicles
			for (size_t i = 0; i < northHeavyVehicles.size(); )
			{
				if (northHeavyVehicles[i]->getY() > 700)
				{
					northHeavyVehicles.erase(northHeavyVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove WEST vehicles
			for (size_t i = 0; i < westVehicles.size(); )
			{
				if (westVehicles[i]->getX() > 700)
				{
					westVehicles.erase(westVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove WEST HEAVY vehicles
			for (size_t i = 0; i < westHeavyVehicles.size(); )
			{
				if (westHeavyVehicles[i]->getX() > 700)
				{
					westHeavyVehicles.erase(westHeavyVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove EAST vehicles
			for (size_t i = 0; i < eastVehicles.size(); )
			{
				if (eastVehicles[i]->getX() < 0)
				{
					eastVehicles.erase(eastVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			// Remove EAST HEAVY vehicles
			for (size_t i = 0; i < eastHeavyVehicles.size(); )
			{
				if (eastHeavyVehicles[i]->getX() < 0)
				{
					eastHeavyVehicles.erase(eastHeavyVehicles.begin() + i);
				}
				else
				{
					i++;
				}
			}

			std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Update vehicles after 50 ms
		}
	}



	//---------------- RENDER VEHICLES and BACKGROUND -----------------//
	void render()
	{
		window.clear();

		// Draw background
		window.draw(backgroundSprite);

		// Draw traffic lights
		southLight.draw(window);
		northLight.draw(window);
		eastLight.draw(window);
		westLight.draw(window);

		// Draw vehicles
		{
			// SOUTH VEHICLES
			for (size_t index = 0; index < southVehicles.size(); ++index)
			{
				southVehicles[index]->draw(window);
			}

			// SOUTH HEAVY VEHICLES
			for (size_t index = 0; index < southHeavyVehicles.size(); ++index)
			{
				southHeavyVehicles[index]->draw(window);
			}

			// NORTH VEHICLES
			for (size_t index = 0; index < northVehicles.size(); ++index)
			{
				northVehicles[index]->draw(window);
			}

			// NORTH HEAVY VEHICLES
			for (size_t index = 0; index < northHeavyVehicles.size(); ++index)
			{
				northHeavyVehicles[index]->draw(window);
			}

			// WEST VEHICLES
			for (size_t index = 0; index < westVehicles.size(); ++index)
			{
				westVehicles[index]->draw(window);
			}

			// WEST HEAVY VEHICLES
			for (size_t index = 0; index < westHeavyVehicles.size(); ++index)
			{
				westHeavyVehicles[index]->draw(window);
			}

			// EAST VEHICLES
			for (size_t index = 0; index < eastVehicles.size(); ++index)
			{
				eastVehicles[index]->draw(window);
			}

			// EAST HEAVY VEHICLES
			for (size_t index = 0; index < eastHeavyVehicles.size(); ++index)
			{
				eastHeavyVehicles[index]->draw(window);
			}
		}

		window.display();
	}

};


// -----------------------------------------------------------------------------------------------------//
// -------------------------------------------- MENU ---------------------------------------------------//
// -----------------------------------------------------------------------------------------------------//

class Menu
{
public:
	std::vector<Vehicle*> vehicles;


	void display()
	{
		sf::RenderWindow window(sf::VideoMode(400, 300), "Traffic Simulation Menu");

		// Load background texture
		sf::Texture backgroundTexture;
		if (!backgroundTexture.loadFromFile("assets/new/background.jpg"))
		{
			std::cerr << "Could not load background image" << std::endl;
			return;
		}
		sf::Sprite background(backgroundTexture);

		while (window.isOpen())
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
					window.close();
				if (event.type == sf::Event::KeyPressed)
				{
					if (event.key.code == sf::Keyboard::Num1)
					{
						runSimulation();
					}
					else if (event.key.code == sf::Keyboard::Num2)
					{
						payChallan();
					}
				}
			}

			window.clear();
			window.draw(background);
			drawMenu(window);
			window.display();
		}
	}

private:
	void drawMenu(sf::RenderWindow& window)
	{
		sf::Font font;
		if (!font.loadFromFile("assets/new/Arial.ttf"))
		{
			std::cerr << "Could not load font" << std::endl;
			return;
		}

		sf::Text title("Traffic Simulation Menu", font, 24);
		title.setFillColor(sf::Color::Yellow);
		title.setPosition(50, 30);
		window.draw(title);

		sf::Text option1("1. Run Simulation", font, 20);
		option1.setFillColor(sf::Color::Green);
		option1.setPosition(50, 100);
		window.draw(option1);

		sf::Text option2("2. Pay Challan", font, 20);
		option2.setFillColor(sf::Color::Red);
		option2.setPosition(50, 150);
		window.draw(option2);
	}

	void runSimulation()
	{
			srand(static_cast<unsigned int>(time(0)));
		
			Simulation trafficSimulation;
			cout << "Traffic Simulaiton Started" << endl;
			trafficSimulation.run();
			cout << "Traffic Simulaiton Ended" << endl;
	}


	void payChallan() {
		sf::RenderWindow window(sf::VideoMode(400, 300), "Pay Challan");

		// Load font
		sf::Font font;
		if (!font.loadFromFile("assets/new/Arial.ttf")) {
			std::cerr << "Could not load font" << std::endl;
			return;
		}

		// Text for entering license number
		sf::Text enterText("Enter License Number: ", font, 20);
		enterText.setFillColor(sf::Color::White);
		enterText.setPosition(20, 50);

		// Text for showing results
		sf::Text resultText("", font, 20);
		resultText.setFillColor(sf::Color::White);
		resultText.setPosition(20, 150);

		std::string licenseNumberStr = "";
		sf::Text licenseInput(licenseNumberStr, font, 20);
		licenseInput.setFillColor(sf::Color::White);
		licenseInput.setPosition(20, 100);

		while (window.isOpen()) {
			sf::Event event;
			while (window.pollEvent(event)) {
				if (event.type == sf::Event::Closed)
					window.close();
				if (event.type == sf::Event::TextEntered) {
					if (event.text.unicode >= '0' && event.text.unicode <= '9') {
						licenseNumberStr += static_cast<char>(event.text.unicode);
					}
					else if (event.text.unicode == '\b' && !licenseNumberStr.empty()) {
						licenseNumberStr.pop_back();
					}
				}
				if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
					bool found = false;
					int licenseNumber = std::stoi(licenseNumberStr);
					for (auto& vehicle : vehicles)
					{
						if (vehicle->getLicenseNumber() == licenseNumber && vehicle->getChallan()) {
							resultText.setString("Vehicle with license number " + licenseNumberStr + " has a challan.");
							found = true;
							break;
						}
					}
					if (!found) {
						resultText.setString("No challan for license number " + licenseNumberStr + ".");
					}
				}
			}

			licenseInput.setString(licenseNumberStr);

			window.clear();
			window.draw(enterText);
			window.draw(licenseInput);
			window.draw(resultText);
			window.display();
		}
	}
};





// -----------------------------------------------------------------------------------------------------//
// -------------------------------------------- MAIN ---------------------------------------------------//
// -----------------------------------------------------------------------------------------------------//

int main()
{
	Menu menu;
	menu.display();
	return 0;
}