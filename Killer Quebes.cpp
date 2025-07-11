#include <iostream>
#include <TL-Engine.h>
#include <vector>
#include <windows.h>	//Sleep() for Countdown Animation
#include <algorithm> // for sort() in positioning
#include <cmath>        // For sqrt and abs
using namespace std;
using namespace tle;
enum class States { Ready, Firing, Contact, Over }; // Game states	
enum class Block_States { first, second, third };
class Block {
private:
	//bool hit{ false };	//now replaced by multi-valued state variable
	Block_States Block_state{ Block_States::first };

public:
	IModel* Block_Model;
	Block(IModel* Block_Model) : Block_state(Block_States::first), Block_Model(nullptr) {}
	Block() : Block_state(Block_States::first), Block_Model(nullptr) {}
	void incrementState() {
		if (Block_state == Block_States::first) {
			Block_state = Block_States::second;
			Block_Model->SetSkin("tiles_bright.jpg");
		}
		else if (Block_state == Block_States::second) {
			Block_state = Block_States::third;
			Block_Model->SetSkin("tiles_green.jpg");
		}
		else if (Block_state == Block_States::third)
			Block_Model->SetY(-11);
	}
	Block_States getState()const { return Block_state; }
};
class Killer_Quebes
{
private:
	I3DEngine* engine;
	ICamera* camera;
	IModel* skybox, * ground;	// Game models/assets
	States Game_State;
	IFont* gameFont, * Small_Font;
	float frameRate, cameraForwardsLimit, cameraBackwardsLimit, cameraLeftLimit, cameraRightLimit, cameraSpeed;
	float rotationSpeed = 15.0f;  // Degrees per second
	float maxRotation = 50.0f;    // Maximum rotation angle in degrees
	float Marble_Rotation = 0.0f; // Track the current rotation around Y axis
	float kGameSpeed;
	vector<vector<Block>> Blocks; // 2D vector of blocks
	IModel* marble, * arrow, * dummy; // Vector to store checkpoints & Isles
	vector<vector<IModel*>> barriers; // 2D vector for two walls
public:
	Killer_Quebes() : Game_State(States::Ready), frameRate(0.0f), cameraForwardsLimit(620.0f), cameraBackwardsLimit(-140.0f), cameraLeftLimit(0.0f), cameraRightLimit(420.0f), cameraSpeed(20.0f)
	{
		kGameSpeed = 20.0f;
		rotationSpeed = 10.0f;  // Degrees per second
		maxRotation = 50.0f;    // Maximum rotation angle in degrees
		engine = New3DEngine(kTLX);
		engine->StartWindowed();
		engine->SetWindowCaption("Killer Quebes");
		engine->AddMediaFolder("./media");
		gameFont = engine->LoadFont("Monotype Corsiva", 45);
		Small_Font = engine->LoadFont("Monotype Corsiva", 35);

		ground = engine->LoadMesh("Floor.x")->CreateModel(0, -5, 0);	//floor model at the default position
		skybox = engine->LoadMesh("Skybox_Hell.x")->CreateModel(0, -1000, 0);

		IMesh* BlocksMesh, * BarriersMesh;
		BlocksMesh = engine->LoadMesh("Block.x"); //PathTransparent
		BarriersMesh = engine->LoadMesh("Barrier.x"); //PathTransparent
		marble = engine->LoadMesh("Marble.x")->CreateModel(0, 5, 0);
		arrow = engine->LoadMesh("Arrow.x")->CreateModel(0, 0, -10);
		dummy = engine->LoadMesh("dummy.x")->CreateModel(0, 5, 0);

		float startZ = 0.0f; // Starting Z position
		float gapZ = 18.0f; // Gap between barriers

		vector<IModel*> rowbarrier1; // Vector for each row
		vector<IModel*> rowbarrier2; // Vector for each row
		for (int i = 0; i < 10; ++i) {
			float zPos = i * gapZ;

			// Create a barrier for the first wall
			IModel* barrier1 = BarriersMesh->CreateModel(70, 0, zPos);
			if (i >= 6)
				barrier1->SetSkin("barrier1.bmp"); // Apply wasp stripes skin
			else
				barrier1->SetSkin("barrier1a.bmp"); // Apply wasp stripes skin

			rowbarrier1.push_back(barrier1);

			// Create a barrier for the second wall
			IModel* barrier2 = BarriersMesh->CreateModel(-70, 0, zPos);
			if (i >= 6)
				barrier2->SetSkin("barrier1.bmp"); // Apply wasp stripes skin
			else
				barrier2->SetSkin("barrier1a.bmp"); // Apply wasp stripes skin

			rowbarrier2.push_back(barrier2);
		}
		barriers.push_back(rowbarrier1);
		barriers.push_back(rowbarrier2);

		float blockWidth = 10.0f;
		float gap = 2.0f;
		float totalWidth = (10 * blockWidth) + (9 * gap);
		float startX = -totalWidth / 2 + blockWidth / 2;
		vector<Block> rowBlocks; // Vector for each row

		for (int i = 0; i < 10; ++i) {
			Block block;
			block.Block_Model = BlocksMesh->CreateModel(startX + i * (blockWidth + gap), 4, 120);
			rowBlocks.push_back(block);
		}
		Blocks.push_back(rowBlocks);
		rowBlocks.clear();
		for (int i = 0; i < 10; ++i) {
			Block block;
			block.Block_Model = BlocksMesh->CreateModel(startX + i * (blockWidth + gap), 4, 132);
			rowBlocks.push_back(block);
		}
		Blocks.push_back(rowBlocks);

		arrow->AttachToParent(dummy);

		camera = engine->CreateCamera(kManual, 0.0f, 30.0f, -60);
		camera->RotateX(10.0f);
		//camera = engine->CreateCamera(kManual, 0.0f, 150, 0);
		//camera->RotateX(90.0f);
	}
	bool SphereBoxcollision(IModel* sphere, IModel* box) {
		// Get the sphere's center and radius
		float sphereCenterX = sphere->GetX();
		float sphereCenterY = sphere->GetY();
		float sphereCenterZ = sphere->GetZ();
		float sphereRadius = 2;

		// Get the box's dimensions
		float boxCenterX = box->GetX();
		float boxCenterY = box->GetY();
		float boxCenterZ = box->GetZ();
		float boxWidth = 10;
		float boxHeight = 10;
		float boxDepth = 10;

		// Calculate the minimum and maximum values of the box's dimensions
		float boxMinX = boxCenterX - boxWidth / 2.0f;
		float boxMinY = boxCenterY - boxHeight / 2.0f;
		float boxMinZ = boxCenterZ - boxDepth / 2.0f;
		float boxMaxX = boxCenterX + boxWidth / 2.0f;
		float boxMaxY = boxCenterY + boxHeight / 2.0f;
		float boxMaxZ = boxCenterZ + boxDepth / 2.0f;

		// Calculate the closest point on the box to the sphere's center
		float closestX = max(boxMinX, min(sphereCenterX, boxMaxX));
		float closestY = max(boxMinY, min(sphereCenterY, boxMaxY));
		float closestZ = max(boxMinZ, min(sphereCenterZ, boxMaxZ));

		// Calculate the distance between the sphere's center and the closest point on the box
		float distanceSquared = (sphereCenterX - closestX) * (sphereCenterX - closestX) +
			(sphereCenterY - closestY) * (sphereCenterY - closestY) +
			(sphereCenterZ - closestZ) * (sphereCenterZ - closestZ);

		// Check if the distance is less than or equal to the sphere's radius
		bool collision = distanceSquared <= sphereRadius * sphereRadius;
		if (collision) {
			// Determine the main axis of collision and calculate the new direction
			float dx = sphereCenterX - boxCenterX;
			float dy = sphereCenterY - boxCenterY;

			float current_Rotation = Marble_Rotation;
			float newRotation;

			if (std::abs(dx) > std::abs(dy)) {
				// Horizontal collision
				if (dx > 0) {
					// Collided with the left side of the box
					newRotation = -current_Rotation + 180.0f;
				}
				else {
					// Collided with the right side of the box
					newRotation = -current_Rotation + 180.0f;
				}
			}
			else {
				// Vertical collision
				if (dy > 0) {
					// Collided with the bottom side of the box
					newRotation = -current_Rotation;
				}
				else {
					// Collided with the top side of the box
					newRotation = -current_Rotation;
				}
			}

			// Ensure the new rotation is within the correct bounds
			newRotation = std::fmod(newRotation, 360.0f);
			if (newRotation < 0)
				newRotation += 360.0f;

			sphere->RotateY(newRotation - current_Rotation); // Apply the rotation difference
		}


		//if (collision) {
		//	// Determine the direction of the collision and adjust the marble's direction accordingly
		//	float dx = sphereCenterX - boxCenterX;
		//	float dy = sphereCenterY - boxCenterY;

		//	// Determine the main axis of collision (horizontal or vertical)
		//	if (std::abs(dx) > std::abs(dy)) {
		//		// Horizontal collision
		//		if (dx > 0) {
		//			// Collided with the left side of the box
		//			sphere->RotateLocalY(180.0f); // Reverse direction
		//		}
		//		else {
		//			// Collided with the right side of the box
		//			sphere->RotateLocalY(180.0f); // Reverse direction
		//		}
		//	}
		//	else {
		//		// Vertical collision
		//		if (dy > 0) {
		//			// Collided with the bottom side of the box
		//			sphere->RotateLocalY(180.0f); // Reverse direction
		//		}
		//		else {
		//			// Collided with the top side of the box
		//			sphere->RotateLocalY(180.0f); // Reverse direction
		//		}
		//	}
		//}		
		//
		return collision;
	}
	void run()
	{
		float rotationAmount;
		bool collision = false, barrier_collision = false;
		//float velocity_X = 0, velocity_Y = 0, velocity_Z = kGameSpeed;
		while (engine->IsRunning())
		{
			frameRate = engine->Timer();
			engine->DrawScene();
			Print_Game_states();

			switch (Game_State)
			{
			case States::Ready: {
				camera->RotateY(engine->GetMouseMovementX() * 0.1f);
				gameFont->Draw("Press space bar to Fire!", 500, 200, kBlack);

				if (engine->KeyHeld(Key_Z))
				{
					if (Marble_Rotation < maxRotation)
					{
						rotationAmount = rotationSpeed * frameRate;
						dummy->RotateLocalY(rotationAmount);
						marble->RotateLocalY(rotationAmount);
						Marble_Rotation += rotationAmount;
					}
				}
				if (engine->KeyHeld(Key_X))
				{
					if (Marble_Rotation > -maxRotation)
					{
						rotationAmount = -rotationSpeed * frameRate;
						dummy->RotateLocalY(rotationAmount);
						marble->RotateLocalY(rotationAmount);
						Marble_Rotation += rotationAmount;
					}
				}
				else if (engine->KeyHit(Key_Space))
					Game_State = States::Firing;

				collision = false;

				break;
			}
			case States::Firing: {
				for (int i = 0; i < 2; i++)
				{
					for (int j = 0; j < 10; j++)
					{
						if (SphereBoxcollision(marble, barriers[i][j]))
							barrier_collision = true;
					}
				}
				if (barrier_collision) {
					//cout << "Going back: " << marble->GetLocalZ() << endl;
					//cout << "velocity_X: " << velocity_X << "  velocity_Y: " << velocity_Y << "  velocity_Z: " << velocity_Z << endl;

					//marble->MoveLocalX(velocity_X);
					//marble->MoveLocalY(velocity_Y * frameRate);
					marble->MoveLocalZ(-kGameSpeed * frameRate);

					if (marble->GetLocalZ() <= 0) {
						cout << "Reached end" << endl;
						barrier_collision = false;
						marble->SetPosition(0, 5, 0);
						marble->ResetOrientation();
						dummy->ResetOrientation();
						Game_State = States::Ready;
					}
				}

				for (int row = 0; row < 2; row++)
				{
					for (int col = 0; col < 10; col++)
					{
						//if (Blocks[row][i].getState() == Block_States::third)
							//continue;
						if (SphereBoxcollision(marble, Blocks[row][col].Block_Model))
						{
							Blocks[row][col].incrementState();
							collision = true;
							break;
						}
					}
				}
				if (collision == false && !barrier_collision) {
					marble->MoveLocalZ(kGameSpeed * frameRate);
					//marble->RotateY(kGameSpeed * frameRate);
					//marble.
				}
				else if (collision) {
					//marble->MoveLocalZ(-(5));
					Game_State = States::Contact;
				}
				if (marble->GetLocalZ() > 150) {
					cout << "Reached end" << endl;
					barrier_collision = false;
					marble->SetPosition(0, 5, 0);
					marble->ResetOrientation();
					dummy->ResetOrientation();
					Game_State = States::Ready;
				}

				//arrow->ResetOrientation();
				break;
			}
			case States::Contact: {
				marble->MoveLocalZ(kGameSpeed * frameRate);
				//marble->MoveLocalX(velocity_X * frameRate);
				//marble->MoveLocalY(velocity_Y * frameRate);
				//marble->MoveLocalZ(velocity_Z * frameRate);
				if (marble->GetLocalZ() <= 0) {
					cout << "Reached end" << endl;
					barrier_collision = false;
					marble->SetPosition(0, 5, 0);
					marble->ResetOrientation();
					dummy->ResetOrientation();
					Game_State = States::Ready;
				}
				if (engine->KeyHit(Key_R))
				{
					marble->SetPosition(0, 5, 0);
					marble->ResetOrientation();
					dummy->ResetOrientation();
					Game_State = States::Ready;
				}
				int hits = 0;
				for (const auto& row : Blocks) {
					for (const auto& block : row) {
						if (block.getState() == Block_States::third) {
							hits++;
						}
					}
				}
				if (hits == 2 * 10)	// All blocks have been deleted
					Game_State = States::Over;
				if (marble->GetLocalZ() > 150) {
					cout << "Reached end" << endl;
					barrier_collision = false;
					marble->SetPosition(0, 5, 0);
					marble->ResetOrientation();
					dummy->ResetOrientation();
					Game_State = States::Ready;
				}
				break;
			}
			case States::Over: {
				break;
			}
			default:
				break;
			}

			if (engine->KeyHit(Key_Up) && camera->GetZ() <= cameraForwardsLimit)
				camera->MoveZ(cameraSpeed);
			if (engine->KeyHit(Key_Down) && camera->GetZ() >= cameraBackwardsLimit)
				camera->MoveZ(-cameraSpeed);
			if (engine->KeyHit(Key_Left) && camera->GetX() >= cameraLeftLimit)
				camera->MoveX(-cameraSpeed);
			if (engine->KeyHit(Key_Right) && camera->GetX() <= cameraRightLimit)
				camera->MoveX(cameraSpeed);

			if (engine->KeyHit(Key_Escape))
				engine->Stop(); // Quit the game //In all states, player should be able to press the Escape key to quit the game
		}
	}
	void Print_Game_states()
	{
		if (Game_State == States::Ready)
			gameFont->Draw("State: Ready", 1075, 670, kWhite);
		else if (Game_State == States::Firing)
			gameFont->Draw("State: Firing", 1050, 670, kWhite);
		else if (Game_State == States::Contact)
			gameFont->Draw("State: Contact", 1050, 670, kWhite);
		else if (Game_State == States::Over)
			gameFont->Draw("State: Complete", 1075, 670, kWhite);
	}
};
int main()
{
	Killer_Quebes game;
	game.run();
	return 0;
}