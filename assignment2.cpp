//G21328027 - D.L.Sasithi Odevni
// assignment2.cpp: A program using the TL-Engine


#include <TL-Engine.h>	// TL-Engine include file and namespace
#include <cmath>
#include <vector>
#include <string>

using namespace tle;

I3DEngine* myEngine;

struct Vector2D
{
	float x, z;

	// Operators for vector addition, subtraction, and scalar multiplication
	Vector2D operator+(const Vector2D& other) const { return { x + other.x, z + other.z }; }
	Vector2D operator-(const Vector2D& other) const { return { x - other.x, z - other.z }; }
	Vector2D operator*(float scalar) const { return { x * scalar, z * scalar }; }

	// Function to calculate the magnitude (length) of the vector
	float Magnitude()
	{
		return sqrt(x * x + z * z);
	}
	void Normalize()   // Normalize the vector (make it a unit vector)
	{
		float magnitude = Magnitude();
		if (magnitude > 0.0f)
		{  
			x /= magnitude;
			z /= magnitude;
		}
	}

};

struct Checkpoint  // Structure for the checkpoint, which includes position, rotation, and collision point
{
	IModel* model;
	float x, y, z;
	float rotation;
	Vector2D collisionPoint;
};
vector<Checkpoint> checkpoints;
size_t currentCheckpointIndex = 0;

std::string checkpointMessage = "";
float countdownTimer = 0.0f;


// Constants related to the game mechanics
const float scale = 100.0;
const float mpsToKmh = 3.6;
const float thrustForce = 0.5f;
const float dragCoefficient = 0.99f;
const float bounceFactor = 0.02f;
const float turnSpeed = 45.0f;
const float maxSpeed = 10.0f;
const float hoverCarRadius = 2.0f;
const float checkpointLegRadius = 1.0f;
const float checkpointLegOffset = 10.0;
const float tankRadius = 2.5f;
const float kPi = 3.14f;
float carRotation = 0.0f;
Vector2D momentum = { 0.0f, 0.0f };


// Camera constants
const float camChaseHeight = 10.0f;
const float camChaseDistance = -30.0f;
const float camFirstPersonHeight = 6.0f;
const float camFirstPersonDistance = 0.0f;

//Boost
const float boostMultiplier = 1.4f;      
const float boostDurationLimit = 3.0f;  
const float boostCooldownTime = 5.0f;    
const float overheatDragMultiplier = 2.0f;
bool boostActive = false; 
bool boostOverheated = false; 
float boostTimer = 0.0f;    
float cooldownTimer = 0.0f; 

//Health Points System
float hoverCarHealth = 100.0f;
const float lowHealthThreshold = 25.0f;
bool isGameOver = false;
bool tookDamage = false;
const float damageAmount = 1.0;

// Mouse movement tracking
int mouseMoveX = 0;
int mouseMoveY = 0;
float cameraAngle = 0.0f;
const float verticalCamAngle = 45.0;
const float kCameraMove = 0.1f;
const float kMouseRotation = 0.1f;
bool mouseCaptureActive = true;

// Camera mode enum
enum CameraMode { Chase, FirstPerson };
CameraMode cameraState = Chase;

// Game state enum
enum gameState { Waiting, Countdown, Racing, Completed };
gameState currentState = Waiting;

// Object structures to store wall, isle, and tank models and positions

struct Object  
{
	IModel* model;
	float x, y, z, rotation;
};

struct ObjectPosition
{
	float x, y, z, rotation;
};

//Function Prototypes

bool CheckpointCrossed(IModel* hoverCar);
bool SphereToBoxCollision(Vector2D spherePos, float sphereRadius, float boxMinX, float boxMaxX, float boxMinZ, float boxMaxZ);
bool SphereToSphereCollision(Vector2D sphere1Pos, float sphere1Radius, Vector2D sphere2Pos, float sphere2Radius);
bool checkCollisionCheckpoint(IModel* hoverCar);
void TakeDamage(float& health, bool& gameOver, float damageAmount, bool& tookDamage);
void HandleCameraMovement(ICamera* myCamera);
void HandleMouseMovement(ICamera* myCamera);
void UpdateCamera(ICamera* camera, IModel* hoverCar);
void CreateIslesAndWalls(IMesh* mesh, vector<Object>& objects, const vector<ObjectPosition>& positions);
void HandleInput(float frameTime, IModel* hoverCar);
void DisplayDialogues(IFont* myFont2, IFont* healthFont, IFont* myFont1);
void HandleCollisions(IModel* hoverCar, Vector2D& momentum, float& hoverCarHealth, bool& isGameOver, bool& tookDamage,
	const vector<Object>& walls, const vector<Object>& isles, const vector<Object>& tanks, const vector<ObjectPosition>& tank1Positions);
void SwitchCameraMode(ICamera* myCamera, IModel* hoverCar);
void UpdateCheckpointMessage(IModel* hoverCar);
void SwitchCameraMode(ICamera* myCamera, IModel* hoverCar);
void HandleWaitingState(IFont* myFont1, ICamera* myCamera, IModel* hoverCar);
void HandleCountdownState(IFont* myFont1, float frameTime);




void main()
{
	// Create a 3D engine (using TLX engine here) and open a window for it
	myEngine = New3DEngine(kTLX);
	myEngine->StartWindowed();

	// Add default folder for meshes and other media
	myEngine->AddMediaFolder(".\\media");

	/**** Set up your scene here ****/
	IMesh* hoverCarMesh = myEngine->LoadMesh("race2.x");
	IModel* hoverCar = hoverCarMesh->CreateModel(0, 0, -30);
	hoverCar->Scale(0.75);

	// Initialize camera and set initial position
	ICamera* myCamera = myEngine->CreateCamera(kManual);
	myCamera->SetPosition(0, camChaseHeight, camChaseDistance);
	myCamera->LookAt(hoverCar);

	//Load Meshes
	IMesh* groundMesh = myEngine->LoadMesh("ground.x");
	IModel* ground = groundMesh->CreateModel();

	IMesh* skyMesh = myEngine->LoadMesh("Skybox 07.x");
	IModel* sky = skyMesh->CreateModel(0, -840, 0);

	IMesh* checkpointMesh = myEngine->LoadMesh("Checkpoint.x");
	IMesh* wallMesh = myEngine->LoadMesh("Wall.x");
	IMesh* isleMesh = myEngine->LoadMesh("IsleStraight.x");
	IMesh* tankMesh = myEngine->LoadMesh("TankSmall2.x");

	vector<Object>  walls, isles, tanks;

	vector<ObjectPosition> checkpointPositions =
	{
	{ 0, 0, 0, 0 },
	{ 10, 0, 120, 90 },
	{ 130, 0, 120, 90 },
	{ 200, 0, 90, 0 },
	{ 25, 0, 56, 0 },
	{ 25, 0, -42, 0 }
	};
	vector<ObjectPosition> wallPositions = 
	{
	{-10, 0, 48, 0}, {10, 0, 48, 0},
	{-10, 0, 64, 0}, {10, 0, 64, 0},
	{62, 0, 110, 90}, {62, 0, 130, 90}, {78, 0, 110, 90}, {78, 0, 130, 90},
	{62, 0, 56, 90}, {78, 0, 56, 90}, {62, 0, 70, 90}, {78, 0, 70, 90},
	{35, 0, -10, 0}, {35, 0, -26, 0}, {15, 0, -10, 0}, {15, 0, -26, 0}
	};
	vector<ObjectPosition> islePositions = 
	{
	{-10, 0, 40, 0}, {10, 0, 40, 0}, {-10, 0, 56, 0}, {10, 0, 56, 0},
	{-10, 0, 72, 0}, {10, 0, 72, 0}, {54, 0, 110, 90}, {54, 0, 130, 90},
	{70, 0, 110, 90}, {70, 0, 130, 90}, {86, 0, 110, 90}, {86, 0, 130, 90},
	{54, 0, 56, 90}, {70, 0, 56, 90}, {86, 0, 56, 90}, {54, 0, 70, 90},
	{70, 0, 70, 90}, {86, 0, 70, 90}, {35, 0, -2, 0}, {35, 0, -18, 0},
	{35, 0, -34, 0}, {15, 0, -2, 0}, {15, 0, -18, 0}, {15, 0, -34, 0}

	};
	vector<ObjectPosition> tankPositions = 
	{
	{10, 0, 5, 0}, {-10, 0, 5, 0}, {10, 0, 15, 0}, {-10, 0, 15, 0},
	{10, 0, 25, 0}, {-10, 0, 25, 0}, {10, 0, 35, 0}, {-10, 0, 35, 0},
	{10, 0, 85, 0}, {-10, 0, 85, 0}, {12, 0, 95, 0}, {-10, 0, 95, 0},
	{14, 0, 105, 0}, {-9, 0, 105, 0}, {-8, 0, 115, 0}, {-7, 0, 125, 0},
	{-4, 0, 130, 0}, {4, 0, 130, 0}, {13, 0, 130, 0}, {43, 0, 110, 0},
	{43, 0, 130, 0}, {33, 0, 110, 0}, {33, 0, 130, 0}, {23, 0, 110, 0},
	{23, 0, 130, 0}, {70, -5, 122, 20}, {95, 0, 130, 0}, {95, 0, 110, 0},
	{105, 0, 130, 0}, {105, 0, 110, 0}, {115, 0, 130, 0}, {115, 0, 110, 0},
	{125, 0, 130, 0}, {125, 0, 110, 0}, {135, 0, 130, 0}, {135, 0, 110, 0},
	{145, 0, 130, 0}, {145, 0, 110, 0}, {155, 0, 130, 0}, {155, 0, 110, 0},
	{165, 0, 130, 0}, {165, 0, 108, 0}, {175, 0, 129, 0}, {175, 0, 105, 0},
	{185, 0, 128, 0}, {182, 0, 100, 0}, {195, 0, 126, 0}, {187, 0, 92, 0},
	{200, 0, 120, 0}, {207, 0, 113, 0}, {207, 0, 105, 0}, {207, 0, 95, 0},
	{187, 0, 82, 0}, {185, 0, 72, 0}, {175, 0, 72, 0}, {165, 0, 72, 0},
	{155, 0, 72, 0}, {145, 0, 72, 0}, {135, 0, 72, 0}, {125, 0, 72, 0},
	{115, 0, 72, 0}, {105, 0, 72, 0}, {95, 0, 72, 0}, {185, 0, 52, 0},
	{175, 0, 52, 0}, {165, 0, 52, 0}, {155, 0, 52, 0}, {145, 0, 52, 0},
	{135, 0, 52, 0}, {125, 0, 52, 0}, {115, 0, 52, 0}, {105, 0, 52, 0},
	{95, 0, 52, 0}, {207, 0, 85, 0}, {205, 0, 75, 0}, {205, 0, 65, 0},
	{200, 0, 58, 0}, {193, 0, 52, 0}, {45, 0, 70, 0}, {45, 0, 56, 0},
	{35, 0, 70, 0}, {35, 0, 56, 0}, {25, 0, 70, 0}, {15, 0, 70, 0},
	{35, 0, 46, 0}, {35, 0, 36, 0}, {35, 0, 26, 0}, {35, 0, 16, 0},
	{35, 0, 6, 0}, {20, -5, 18, 20}
	};

	// Create models for objects
	for (size_t i = 0; i < checkpointPositions.size(); i++)
	{
		IModel* model = checkpointMesh->CreateModel(checkpointPositions[i].x, checkpointPositions[i].y, checkpointPositions[i].z);

		model->RotateY(checkpointPositions[i].rotation);

		checkpoints.push_back({ model, checkpointPositions[i].x, checkpointPositions[i].y, checkpointPositions[i].z, checkpointPositions[i].rotation });

	}

	
	for (size_t i = 0; i < tankPositions.size(); i++)
	{
		IModel* model = tankMesh->CreateModel(tankPositions[i].x, tankPositions[i].y, tankPositions[i].z);

		model->RotateX(tankPositions[i].rotation);

		tanks.push_back({ model, tankPositions[i].x, tankPositions[i].y, tankPositions[i].z, tankPositions[i].rotation });
	}

	CreateIslesAndWalls(wallMesh, walls, wallPositions);
	CreateIslesAndWalls(isleMesh, isles, islePositions);

	// Create UI elements
	ISprite* uiBackdrop = myEngine->CreateSprite("ui_backdrop.jpg", 0, myEngine->GetHeight() - 100);
	IFont* myFont1 = myEngine->LoadFont("Digital-7", 30);
	IFont* myFont2 = myEngine->LoadFont("Digital-7", 20);
	IFont* healthFont = myEngine->LoadFont("Digital-7", 18);

	myEngine->StartMouseCapture();

	float frameTime = myEngine->Timer();

	// The main game loop, repeat until engine is stopped
	while (myEngine->IsRunning())
	{
		// Draw the scene
		myEngine->DrawScene();
		frameTime = myEngine->Timer();

		// Handle camera movement and game state
		HandleCameraMovement(myCamera);

		switch (currentState)
		{
		case Waiting:
			HandleWaitingState(myFont1, myCamera, hoverCar);
			break;

		case Countdown:
			HandleCountdownState( myFont1,  frameTime);
			break;

		case Racing:
		{
			HandleInput( frameTime,  hoverCar);
			DisplayDialogues(myFont2, healthFont, myFont1);
			HandleCollisions(hoverCar, momentum, hoverCarHealth, isGameOver, tookDamage, walls, isles, tanks, tankPositions);
			SwitchCameraMode( myCamera,  hoverCar);
			UpdateCheckpointMessage(hoverCar);
			SwitchCameraMode(myCamera, hoverCar);
			if (mouseCaptureActive)
			{
				HandleMouseMovement(myCamera);
			}
		}
		break;

		case Completed:
			myFont1->Draw("Game Over!", 200, 610, kRed);

		}

		if (myEngine->KeyHit(Key_Escape))
		{
			myEngine->Stop();
		}

		/**** Update your scene each frame here ****/

	}

	// Delete the 3D engine now we are finished with it
	myEngine->Delete();
}

//Function Definitions 

bool CheckpointCrossed(IModel* hoverCar)
{
	for (size_t i = 0; i < checkpoints.size(); ++i)
	{
		if (currentCheckpointIndex < checkpoints.size())

		{
			float hoverCarX = hoverCar->GetX();
			float hoverCarZ = hoverCar->GetZ();

			float checkpointX = checkpoints[currentCheckpointIndex].x;
			float checkpointZ = checkpoints[currentCheckpointIndex].z;


			const float boxSize = 20.0f;

			float halfWidth = boxSize / 2;
			float halfDepth = boxSize / 10;

			if (checkpoints[i].rotation == 0)
			{
				bool isInside = (hoverCarX >= checkpointX - halfDepth && hoverCarX <= checkpointX + halfDepth) &&
					(hoverCarZ >= checkpointZ - halfWidth && hoverCarZ <= checkpointZ + halfWidth);
				return isInside;

			}
			else if (checkpoints[i].rotation == 90)
			{
				bool isInside = (hoverCarX >= checkpointX - halfWidth && hoverCarX <= checkpointX + halfWidth) &&
					(hoverCarZ >= checkpointZ - halfDepth && hoverCarZ <= checkpointZ + halfDepth);
				return isInside;

			}

		}
	}
	return false;
}
bool SphereToBoxCollision(Vector2D spherePos, float sphereRadius, float boxMinX, float boxMaxX, float boxMinZ, float boxMaxZ)
{

	float closestX;
	if (spherePos.x < boxMinX) 
	{
		closestX = boxMinX;
	}
	else if (spherePos.x > boxMaxX) 
	{
		closestX = boxMaxX;
	}
	else {
		closestX = spherePos.x;
	}

	float closestZ;
	if (spherePos.z < boxMinZ)
	{
		closestZ = boxMinZ;
	}
	else if (spherePos.z > boxMaxZ)
	{
		closestZ = boxMaxZ;
	}
	else {
		closestZ = spherePos.z;
	}

	float distanceX = spherePos.x - closestX;
	float distanceZ = spherePos.z - closestZ;


	return (distanceX * distanceX + distanceZ * distanceZ) < (sphereRadius * sphereRadius);
}

bool SphereToSphereCollision(Vector2D sphere1Pos, float sphere1Radius, Vector2D sphere2Pos, float sphere2Radius) 
{
	float distanceX = sphere1Pos.x - sphere2Pos.x;
	float distanceZ = sphere1Pos.z - sphere2Pos.z;

	float distanceSquared = distanceX * distanceX + distanceZ * distanceZ;
	float radiusSum = sphere1Radius + sphere2Radius;

	return distanceSquared < (radiusSum * radiusSum);
}
bool checkCollisionCheckpoint(IModel* hoverCar)
{
	float hoverCarX = hoverCar->GetX();
	float hoverCarZ = hoverCar->GetZ();

	float checkpointLegLX, checkpointLegRX, checkpointLegZL, checkpointLegZR;
	float distanceLeftLeg, distanceRightLeg;

	for (size_t i = 0; i < checkpoints.size(); ++i)
	{
		if (checkpoints[i].rotation == 0)
		{
			checkpointLegLX = checkpoints[i].x + checkpointLegOffset;
			checkpointLegRX = checkpoints[i].x - checkpointLegOffset;
			checkpointLegZL = checkpointLegZR = checkpoints[i].z;

			distanceLeftLeg = sqrt((hoverCarX - checkpointLegLX) * (hoverCarX - checkpointLegLX) +
				(hoverCarZ - checkpointLegZL) * (hoverCarZ - checkpointLegZL));

			distanceRightLeg = sqrt((hoverCarX - checkpointLegRX) * (hoverCarX - checkpointLegRX) +
				(hoverCarZ - checkpointLegZR) * (hoverCarZ - checkpointLegZR));
		}
		else if (checkpoints[i].rotation == 90)
		{

			checkpointLegLX = checkpoints[i].x;
			checkpointLegRX = checkpoints[i].x;
			checkpointLegZL = checkpoints[i].z + checkpointLegOffset;
			checkpointLegZR = checkpoints[i].z - checkpointLegOffset;

			distanceLeftLeg = sqrt((hoverCarX - checkpointLegLX) * (hoverCarX - checkpointLegLX) +
				(hoverCarZ - checkpointLegZL) * (hoverCarZ - checkpointLegZL));

			distanceRightLeg = sqrt((hoverCarX - checkpointLegRX) * (hoverCarX - checkpointLegRX) +
				(hoverCarZ - checkpointLegZR) * (hoverCarZ - checkpointLegZR));
		}

		if (distanceLeftLeg < (hoverCarRadius + checkpointLegRadius))
		{
			float factor = hoverCarRadius / (hoverCarRadius + checkpointLegRadius);
			checkpoints[i].collisionPoint.x = hoverCarX + (checkpointLegLX - hoverCarX) * factor;
			checkpoints[i].collisionPoint.z = hoverCarZ + (checkpointLegZL - hoverCarZ) * factor;
			return true;
		}
		if (distanceRightLeg < (hoverCarRadius + checkpointLegRadius))
		{
			float factor = hoverCarRadius / (hoverCarRadius + checkpointLegRadius);
			checkpoints[i].collisionPoint.x = hoverCarX + (checkpointLegRX - hoverCarX) * factor;
			checkpoints[i].collisionPoint.z = hoverCarZ + (checkpointLegZR - hoverCarZ) * factor;
			return true;
		}
	}
	return false;
}
void TakeDamage(float& health, bool& gameOver, float damageAmount, bool& tookDamage)
{
	if (!tookDamage) // Prevents continuous damage application
	{
		if (health > 0)
		{
			health -= damageAmount;
		}
		if (health <= 0)
		{
			health = 0;
			gameOver = true;
		}
		tookDamage = true;
	}
}

void HandleCameraMovement(ICamera* myCamera)
{
	if (myEngine->KeyHeld(Key_Up))    myCamera->MoveLocalZ(kCameraMove);
	if (myEngine->KeyHeld(Key_Down))  myCamera->MoveLocalZ(-kCameraMove);
	if (myEngine->KeyHeld(Key_Right)) myCamera->MoveLocalX(kCameraMove);
	if (myEngine->KeyHeld(Key_Left))  myCamera->MoveLocalX(-kCameraMove);

}

void HandleMouseMovement(ICamera* myCamera)
{
	float mouseDeltaX = myEngine->GetMouseMovementX();
	float mouseDeltaY = myEngine->GetMouseMovementY();

	float adjustedMouseRotation = kMouseRotation; // determines how fast the camera rotates when the mouse moves

	myCamera->RotateY(mouseDeltaX * adjustedMouseRotation);


	float newCamAngle = cameraAngle + (mouseDeltaY * adjustedMouseRotation);
	if (newCamAngle > verticalCamAngle) newCamAngle = verticalCamAngle;
	if (newCamAngle < -verticalCamAngle) newCamAngle = -verticalCamAngle;

	float pitchChange = newCamAngle - cameraAngle;
	myCamera->RotateLocalX(pitchChange);

	cameraAngle = newCamAngle;
}

void UpdateCamera(ICamera* camera, IModel* hoverCar)
{
	if (cameraState == Chase)
	{
		camera->AttachToParent(hoverCar);
		camera->SetLocalPosition(0, camChaseHeight, camChaseDistance);
		camera->LookAt(hoverCar);

	}
	else if (cameraState == FirstPerson)
	{
		camera->AttachToParent(hoverCar);
		camera->SetLocalPosition(0, camFirstPersonHeight, camFirstPersonDistance);
		camera->ResetOrientation();
	}
}

void CreateIslesAndWalls(IMesh* mesh, vector<Object>& objects, const vector<ObjectPosition>& positions)
{
	for (size_t i = 0; i < positions.size(); i++)
	{
		IModel* model = mesh->CreateModel(positions[i].x, positions[i].y, positions[i].z);
		model->RotateY(positions[i].rotation);  

		objects.push_back({ model, positions[i].x, positions[i].y, positions[i].z, positions[i].rotation });
	}
}
void HandleInput(float frameTime, IModel* hoverCar)
{

	float directionX = sin(carRotation * kPi / 180);
	float directionZ = cos(carRotation * kPi / 180);

	float speed = momentum.Magnitude();
	float maxBackwardSpeed = maxSpeed / 2;

	Vector2D thrust = { 0.0f, 0.0f };

	if (myEngine->KeyHeld(Key_W))
	{

		float appliedThrust = thrustForce;

		if (myEngine->KeyHeld(Key_Space) && !boostOverheated && hoverCarHealth >= lowHealthThreshold)
		{
			boostActive = true;
			appliedThrust *= boostMultiplier;  // Increase thrust
			boostTimer += frameTime;		  // Track boost duration
		}
		else
		{
			boostActive = false;

			if (!boostOverheated)
			{
				boostTimer = 0.0f;
			} // Reset timer if boost is not used
		}


		if (momentum.Magnitude() < maxSpeed)
		{
			if (!boostOverheated)
			{
				thrust.x = appliedThrust * directionX * frameTime;
				thrust.z = appliedThrust * directionZ * frameTime;
			}
			else
			{

				thrust.x = thrustForce * directionX * frameTime;
				thrust.z = thrustForce * directionZ * frameTime;
			}
		}
		else
		{
			speed = maxSpeed;
		}

		if (boostActive && boostTimer >= boostDurationLimit)
		{
			boostOverheated = true;
			cooldownTimer = 0.0f;
		}

		if (boostOverheated)
		{
			momentum.x *= (1.0f - frameTime * (dragCoefficient * boostMultiplier));
			momentum.z *= (1.0f - frameTime * (dragCoefficient * boostMultiplier));

			// Handle cooldown timer
			cooldownTimer += frameTime;
			if (cooldownTimer >= boostCooldownTime)
			{
				boostOverheated = false;
				boostTimer = 0.0f;
			}
		}

	}
	if (myEngine->KeyHeld(Key_S))
	{
		float appliedThrust = -thrustForce / 2;
		if (speed > -maxBackwardSpeed)
		{
			thrust.x = appliedThrust * directionX * frameTime;
			thrust.z = appliedThrust * directionZ * frameTime;
		}
		else
		{
			speed = -maxBackwardSpeed;
		}
	}
	momentum = momentum + thrust;

	// Apply drag only if  not at max speed
	if (momentum.Magnitude() < maxSpeed)
	{
		momentum = momentum * dragCoefficient; 
	}
	else
	{
		speed = maxSpeed;
	}

	float currentSpeed = momentum.Magnitude();

	if (currentSpeed > maxSpeed)
	{
		// Normalize the momentum and scale it to the max speed
		momentum.Normalize();
		momentum = momentum * maxSpeed;
	}
	

	if (currentSpeed < 0.0f)
	{
		momentum.x = 0.0f;
		momentum.z = 0.0f;
	}


	hoverCar->Move(momentum.x, 0, momentum.z);

	if (myEngine->KeyHeld(Key_A))
	{
		carRotation -= turnSpeed * frameTime;
		hoverCar->RotateY(-turnSpeed * frameTime);
	}

	if (myEngine->KeyHeld(Key_D))
	{
		carRotation += turnSpeed * frameTime;
		hoverCar->RotateY(+turnSpeed * frameTime);
	}


	
}

void DisplayDialogues(IFont* myFont2, IFont* healthFont, IFont* myFont1)
{
	float speed;
	speed = momentum.Magnitude();

	float speed_mps = speed *scale;

	float speed_kph = speed_mps * mpsToKmh; // Convert m/s to km/h
	
	int speedDisplay = std::round(speed_kph);
	myFont2->Draw("Speed: " + std::to_string(speedDisplay) + " km/h", 50, 600, kBlue);

	std::string healthText = "Health: " + std::to_string((int)hoverCarHealth);
	healthFont->Draw(healthText, 50, 620, kRed);

	std::string boostText = "Boost: ";
	if (boostOverheated)
	{
		boostText += "OVERHEATED!";
		myFont2->Draw(boostText, 50, 640, kRed); 
	}
	else if (boostActive)
	{
		boostText += "ACTIVE";
		myFont2->Draw(boostText, 50, 640, kBlue); 

		if (boostTimer >= boostDurationLimit - 1.0f) // Show warning 1 sec before overheat
		{
			myFont2->Draw("WARNING: Boost Overheating!", 200, 640, kRed);
		}
	}
	else
	{
		boostText += "Ready";
		myFont2->Draw(boostText, 50, 640, kGreen); 
	}
	if (hoverCarHealth < lowHealthThreshold)  // If health is below 25% displaying a warning
	{
		std::string healthWarning = "Warning: Low Health!";
		myFont2->Draw(healthWarning, 210, 640, kYellow); 
	}

	if (!checkpointMessage.empty())
	{
		myFont1->Draw(checkpointMessage, 230, 610);
	}


}
void HandleCollisions(IModel* hoverCar, Vector2D& momentum, float& hoverCarHealth, bool& isGameOver, bool& tookDamage,
	const vector<Object>& walls, const vector<Object>& isles, const vector<Object>& tanks, const vector<ObjectPosition>& tankPositions)
{
	bool collided = false;
	Vector2D bounceDirection = { 0.0f, 0.0f };

	for (size_t i = 0; i < checkpoints.size(); ++i)
	{

		Vector2D bounceDirection = { hoverCar->GetX() - checkpoints[i].collisionPoint.x, 
			hoverCar->GetZ() - checkpoints[i].collisionPoint.z };
		if (checkCollisionCheckpoint(hoverCar))
		{

			bounceDirection.Normalize();

			momentum = bounceDirection * bounceFactor;
			if (!tookDamage)
			{
				TakeDamage(hoverCarHealth, isGameOver, damageAmount, tookDamage);
			}
		}
		else
		{
			tookDamage = false;
		}

	}



	for (size_t i = 0; i < walls.size(); ++i)
	{
		Vector2D carPosition = { hoverCar->GetX(), hoverCar->GetZ() };
		if (SphereToBoxCollision(carPosition, hoverCarRadius, walls[i].x - hoverCarRadius, walls[i].x +
			hoverCarRadius, walls[i].z - hoverCarRadius, walls[i].z + hoverCarRadius))
		{
			collided = true;
			bounceDirection = { hoverCar->GetX() - walls[i].x, hoverCar->GetZ() - walls[i].z };
			if (!tookDamage)
			{
				TakeDamage(hoverCarHealth, isGameOver, damageAmount, tookDamage);
			}
			break;
		}
	}

	for (size_t i = 0; i < isles.size(); ++i)
	{
		Vector2D carPosition = { hoverCar->GetX(), hoverCar->GetZ() };
		if (SphereToBoxCollision(carPosition, hoverCarRadius, isles[i].x - hoverCarRadius, isles[i].x +
			hoverCarRadius, isles[i].z - hoverCarRadius, isles[i].z + hoverCarRadius))
		{
			collided = true;
			bounceDirection = { hoverCar->GetX() - isles[i].x, hoverCar->GetZ() - isles[i].z };
			if (!tookDamage)
			{
				TakeDamage(hoverCarHealth, isGameOver, damageAmount, tookDamage);
			}
			break;
		}
	}

	for (size_t i = 0; i < tanks.size(); ++i)
	{
		Vector2D carPosition = { hoverCar->GetX(), hoverCar->GetZ() };
		Vector2D tankPosition = { tankPositions[i].x, tankPositions[i].z };

		if (SphereToSphereCollision(carPosition, hoverCarRadius, tankPosition, tankRadius))
		{
			collided = true;
			bounceDirection = { hoverCar->GetX() - tanks[i].x, hoverCar->GetZ() - tanks[i].z };
			if (!tookDamage)
			{
				TakeDamage(hoverCarHealth, isGameOver, damageAmount, tookDamage);
			}
			break;
		}
	}

	if (collided)
	{
		
		Vector2D bounce = bounceDirection * bounceFactor;

		momentum = momentum + bounce;
	}

}

void SwitchCameraMode(ICamera* myCamera, IModel* hoverCar)
{
	if (myEngine->KeyHit(Key_1))  // Switch to Chase Camera
	{
		cameraState = Chase;
		UpdateCamera(myCamera, hoverCar);

	}

	if (myEngine->KeyHit(Key_2))  // Switch to First-Person Camera
	{
		cameraState = FirstPerson;
		UpdateCamera(myCamera, hoverCar);
	}


	if (myEngine->KeyHit(Key_Tab))
	{
		mouseCaptureActive = !mouseCaptureActive;

		if (mouseCaptureActive)
		{
			myEngine->StartMouseCapture();
		}
		else
		{
			myEngine->StopMouseCapture();
		}

	}
}

void UpdateCheckpointMessage(IModel* hoverCar)
{
	if (CheckpointCrossed(hoverCar))
	{
		currentCheckpointIndex++;

		if (currentCheckpointIndex == checkpoints.size())
		{
			checkpointMessage = "Race Complete!";
		}
		else if (currentCheckpointIndex == 1)
		{
			checkpointMessage = "Stage 1 Complete!";
		}
		else if (currentCheckpointIndex == 2)
		{
			checkpointMessage = "Stage 2 Complete!";
		}
		else if (currentCheckpointIndex == 3)
		{
			checkpointMessage = "Stage 3 Complete!";
		}

		else if (currentCheckpointIndex == 4)
		{
			checkpointMessage = "Stage 4 Complete!";
		}
		else if (currentCheckpointIndex == 5)
		{
			checkpointMessage = "Stage 5 Complete!";
		}


	}
	if (isGameOver)
	{
		checkpointMessage = "";
		currentState = Completed;
	}

}
void HandleWaitingState(IFont* myFont1, ICamera* myCamera , IModel* hoverCar)
{
	myFont1->Draw("Hit Space to Start", 200, 610);
	UpdateCamera(myCamera, hoverCar);


	if (myEngine->KeyHit(Key_Space))
	{
		currentState = Countdown;
		countdownTimer = 3.0f;

	}
}

void HandleCountdownState(IFont* myFont1, float frameTime)
{
	countdownTimer -= frameTime;

	if (countdownTimer > 2.0f)
	{
		myFont1->Draw("3", 200, 610);
	}
	else if (countdownTimer > 1.0f)
	{
		myFont1->Draw("2", 200, 610);
	}
	else if (countdownTimer > 0.0f)
	{
		myFont1->Draw("1", 200, 610);
	}
	else if (countdownTimer > -0.5f)
	{
		myFont1->Draw("Go!", 200, 610);
	}
	else
	{
		currentState = Racing;
		countdownTimer = 0.0f;
	}
}