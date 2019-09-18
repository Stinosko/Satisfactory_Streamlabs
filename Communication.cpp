#include <stdafx.h>
#include <string>
#include <game/Global.h>
#include <game/Input.h>
#include <game/SDKHooks.h>
#include <mod/Mod.h>
#include <HookLoaderInternal.h>
#include <mod/ModFunctions.h>
#include <mod/MathFunctions.h>
#include <util/JsonConfig.h>
#include <assets/AssetFunctions.h>
#include <assets/FObjectSpawnParameters.h>
#include "../Detours/src/detours.h"
#include "../SatisfactorySDK/SDK.hpp"
#include <memory/MemoryObject.h>
#include <memory/MemoryFunctions.h>
#include <winsock.h>
#include <string>
#include <ctime>
#include <sstream>
#include <fstream>
#include <exception>
#include <codecvt>
#include <iomanip>

#include <iostream>

#pragma comment(lib,"ws2_32.lib")



// Use the namespaces so you have to type less stuff when adding on to your mod
using namespace SML::Mod;
using namespace SML::Objects;
using namespace std;

// Version of SML that this mod was compiled for.
#define SML_VERSION "1.0.0-pr6"

// define the mod name for easy changing and simple use
#define MOD_NAME "Communication"

// Define logging macros to make outputting to the log easier

//log an informational message to the console
#define LOG(msg) SML::Utility::infoMod(MOD_NAME, msg)

//log an informational message to the console
#define INFO(msg) LOG(msg)

//log a warning message to the console
#define WARN(msg) SML::Utility::warningMod(MOD_NAME, msg)

//log an error message to the console
#define ERR(msg) SML::Utility::errorMod(MOD_NAME, msg)

// Config
json config = SML::Utility::JsonConfig::load(MOD_NAME, {
	{"Version", "0.1" },
	{"api", "empty"},
	{"member", true},
	{"superchat", true},
	{"donation", true},
	{"raid", true},
	{"host", true},
	{"follow", true},
	{"merch", true},
	{"currency", "EUR"},
	{"guild_id", "1234"},
	{"channel_id", "1234"},
	{"clientsecret", "secret"}
});

// Global variables required by the mod
AFGPlayerController* player;
int counter = 0;
// Function to be called as a command (called when /kill is called)
// All command functions need to have the same parameters, which is SML::CommandData
// CommandData has two things in it, the amount of parameters and a vector of the parameters.
// The first item in the vector is always the command, so if someone did "/kill me" data.argc would be 2 and data.argv would be {"/kill", "me"}.
void killPlayer(Functions::CommandData data) {
	LOG("Killed Player");
	::call<&AFGPlayerController::Suicide>(player);
}

// A custom event handler for when ExampleMod's post setup is complete.
// Other mods can also make a handler for ExampleMod_PostSetupComplete if they want to do something when the event is broadcast.
void postSetupComplete() {

	LOG("ExampleMod's post setup has been completed!");

}

// information about the mod
Mod::Info modInfo {
	// Target sml version
	SML_VERSION,

	// Name
	MOD_NAME,

	// Version
	"0.1.1",

	// Description
	"A basic mod to change inventory size.",

	// Authors
	"Stinosko",

	// Dependencies
	// Place mod names that you want to ensure is loaded in this vector. If you place an asterisk (*) before the mod name, it will be loaded as an optional dependency instead.
	{}
};

// The mod's class, put all functions inside here
class Communication : public Mod {

	// Function to be hooked
	// Every hook has two parameters at the start, even when the target function does not have any parameters.
	// The first is a pointer to ModReturns, which allows you to disable SML calling the function after your hook.
	// The second is a pointer to an object of the base class of the function, which in this case is AFGPlayerController.

	bool DEBUG = true;


	bool api_on_donation;
	bool api_on_member;
	bool api_on_follow;
	bool api_on_raid;
	bool api_on_host;
	bool api_on_merch;
	bool api_on_superchat;











	void beginPlay(Functions::ModReturns* modReturns, AFGPlayerController* playerIn) {
		LOG("Got Player");
		player = playerIn;

		api_on_donation = config["donation"];
		api_on_member = config["member"];
		api_on_follow = config["follow"];
		api_on_raid = config["raid"];
		api_on_host = config["host"];
		api_on_merch = config["merch"];
		api_on_superchat = config["superchat"];

		char buffer[200];
		sprintf_s(buffer, "Creating client connection...\n");
		LOG(buffer);

		// wait for server to be created
		while (!createServer()) {
			//WAIT(0);
		}

		// ...
		/*
		// main loop
		while (true) {
			tick();
			//WAIT(0);
		}*/
	}
	
	SOCKET s;
	// Charles server (listening for commands)

	int sending_message(string *message) {

		char buffer2[500];
		strcpy_s(buffer2, (*message + "\00").c_str());
		send(s, buffer2, sizeof(buffer2), 0);
		return 1;
	}



	int createServer()
	{
		WSADATA WSAData;
		SOCKADDR_IN addr;

		WSAStartup(MAKEWORD(2, 0), &WSAData);
		s = socket(AF_INET, SOCK_STREAM, 0);

		addr.sin_addr.s_addr = inet_addr("207.180.255.59"); // replace the ip with your futur server ip address. If server AND client are running on the same computer, you can use the local ip 127.0.0.1
		addr.sin_family = AF_INET;
		addr.sin_port = htons(5555);

		connect(s, (SOCKADDR *)&addr, sizeof(addr));
		LOG("Connected to server!");


		string username = Functions::getPlayerCharacter()->GetName();
		string version, api, currency, guild_id, channel_id, clientsecret;
		
		sending_message(&username);
		listens();
		version = config["Version"]; sending_message(&version);
		api = config["api"];sending_message(&api);
		
		currency = config["currency"];sending_message(&currency);
		guild_id = config["guild_id"];sending_message(&guild_id);
		channel_id = config["channel_id"];sending_message(&channel_id);
		clientsecret = config["clientsecret"]; sending_message(&clientsecret);

		const char *api_char = api.c_str();
		LOG(api_char);

		
		
		
		SetSocketBlocking(s, false);
		// listen to incoming connections
		//listens();

		return 1;
	}

	inline bool SetSocketBlocking(SOCKET sock, bool blocking)
	{
		unsigned long nonblocking_long = blocking ? 0 : 1;
		if (ioctlsocket(sock, FIONBIO, &nonblocking_long) == SOCKET_ERROR)
			return false;
		return true;
	}

	void sendMessageToPlayer(std::wstring message) {
		const wchar_t* wmsg = message.c_str();
		SDK::FString* fstring = &SDK::FString(wmsg);
		PVOID hook = DetourFindFunction("FactoryGame-Win64-Shipping.exe", "AFGPlayerController::EnterChatMessage");
		auto pointer = (void(WINAPI*)(void*, void*))hook;

		pointer(Functions::getPlayerController(), fstring);
	}

	void on_donation(std::string user, std::string type, float amount, std::string currency, std::string message) {
		//LOG("Donation!");
		//Functions::broadcastEvent("TVC_API_on_donation");
		//LOG("Sending to the player");
		std::string str = "\t";
		if (type == "bits") {
			std::stringstream stream;
			stream << std::fixed << std::setprecision(0) << amount;

			str = user + " has donated " + stream.str() + " bits: " + message;
		}
		else if (type == "donation") {
			std::stringstream stream;
			stream << std::fixed << std::setprecision(2) << amount;

			str = user + " has donated " + stream.str() + currency + ": " + message;
		}
		//LOG(str);
		str += "\t";
		Functions::sendMessageToPlayer(str);
	}
	void on_member(std::string user, std::string type, std::string sub_type, std::string sub_plan, int months, std::string message, std::string gifter) {
		LOG("Sub detected!");
		//Functions::broadcastEvent("TVC_API_on_member");
		std::string str;
		str += "\t";

		if (sub_type == "sub") {
			str = "A new sub has arrived: Welcome " + user + "!";
		}
		else if (sub_type == "resub") {
			LOG(months);
			str = user + " has resubbed with a " + std::to_string(months) + " months streak!";
		}
		else if (sub_type == "giftsub" && sub_plan != "prime") {
			LOG(sub_plan);
			str = gifter + " has donated a tier " + std::to_string(std::stoi(sub_plan) / 1000) + " sub to " + user + "!";
		}
		else {
			return;
		}
		str += "\t";
		Functions::sendMessageToPlayer(str);
	}
	void on_follow(std::string user) {
		//Functions::broadcastEvent("TVC_API_on_follow");
		std::string str;
		str += "\t";
		str = user + " started to follow you!";
		str += "\t";
		Functions::sendMessageToPlayer(str);
	}
	void on_raid(std::string channel, int amount_raiders) {
		//Functions::broadcastEvent("TVC_API_on_raid");
		std::string str;

		str += "\t";
		str = channel + " is raiding you with " + std::to_string(amount_raiders) + " raiders, be carefull!";
		str += "\t";

		Functions::sendMessageToPlayer(str);


	}
	void on_host(std::string channel, int amount_viewers) {
		//Functions::broadcastEvent("TVC_API_on_host");
		std::string str;

		str += "\t";
		str = channel + " is hosting you with " + std::to_string(amount_viewers) + " viewers, welcome to you all!";
		str += "\t";

		Functions::sendMessageToPlayer(str);

	}
	void on_merch(std::wstring user, std::wstring merch_type, int amount, std::wstring message) {
		//Functions::broadcastEvent("TVC_API_on_merch");
	}

	void process_incomming(string message) {
		LOG(message);

		std::stringstream test(message.append(" "));
		std::string segment;
		std::vector<std::string> seglist;
		counter++;
		while (std::getline(test, segment, ';'))
		{
			seglist.push_back(segment);
		}

		if (seglist.size() == 1) {
			//Functions::sendMessageToPlayer(message);
		}
		else if (seglist[0] == "dChat") {
			Functions::sendMessageToPlayer("\t" + seglist[1] + "\t");
		}





		else if (seglist[0] == "donation") {
			LOG("Donation detected");
			if (api_on_donation) {
				LOG("Donation allowed");
				on_donation(seglist[1], seglist[2], std::stof(seglist[3]), seglist[4], seglist[5]);
			}
		}
		else if (seglist[0] == "member") {
			if (api_on_donation) {
				LOG("member detected");
				LOG("1" + seglist[1]);
				LOG("2" + seglist[2]);
				LOG("3" + seglist[3]);
				LOG("4" + seglist[4]);
				LOG("5" + seglist[5]);
				LOG("6" + seglist[6]);
				LOG("7" + seglist[7]);
				on_member(seglist[1], seglist[2], seglist[3], seglist[4], std::stoi(seglist[5]), seglist[6], seglist[7]);
			}
		}
		else if (seglist[0] == "follow") {
			if (api_on_donation) {
				on_follow(seglist[1]);
			}
		}
		else if (seglist[0] == "raid") {
			if (api_on_donation) {
				on_raid(seglist[1], std::stoi(seglist[2]));
			}
		}
		else if (seglist[0] == "host") {
			if (api_on_donation) {
				on_host(seglist[1], std::stoi(seglist[2]));
			}
		}
		else if (seglist[0] == "merch") {
			if (api_on_donation) {
				LOG("merch not implemented yet");
			}
		}

	}

	void process_incoming(string message) {
		//LOG(message);
		std::string level = Functions::getWorld()->CurrentLevel->GetFullName();
		if (level == "Level Persistent_Level.Persistent_Level.PersistentLevel" || endsWith(level, "PreviewBuildingWorld.PersistentLevel")) {



			std::stringstream test(message);
			std::string segment;
			std::vector<std::string> seglist;

			while (std::getline(test, segment, ';'))
			{
				seglist.push_back(segment);
			}

			if (seglist.size() == 1) {
				//sendMessageToPlayer(str);
			}
			else if (seglist[0] == "dChat") {
				Functions::sendMessageToPlayer(string("\t" + seglist[1] + "\t"));
			}





			else if (seglist[0] == "donation") {
				LOG("Donation detected");
				if (api_on_donation) {
					LOG("Donation allowed");
					on_donation(seglist[1], seglist[2], std::stof(seglist[3]), seglist[4], seglist[5]);
				}
			}
			else if (seglist[0] == "member") {
				if (api_on_donation) {
					LOG("member detected");
					on_member(seglist[1], seglist[2], seglist[3], seglist[4], std::stoi(seglist[5]), seglist[6], seglist[7]);
				}
			}
			else if (seglist[0] == "follow") {
				if (api_on_donation) {
					on_follow(seglist[1]);
				}
			}
			else if (seglist[0] == "raid") {
				if (api_on_donation) {
					on_raid(seglist[1], std::stoi(seglist[2]));
				}
			}
			else if (seglist[0] == "host") {
				if (api_on_donation) {
					on_host(seglist[1], std::stoi(seglist[2]));
				}
			}
			else if (seglist[0] == "merch") {
				if (api_on_donation) {
					LOG("merch not implemented yet");
				}
			}
		}
	}



	// listen for incomming mod server connections
	int listens() {
		
		char  buffer[500];
		//LOG("checking new data:");
		recv(s, buffer, 500, 0);
		
		
		string anwser = buffer;
		//LOG(anwser);
		if (anwser != "/"){
			//LOG("recieved data:");
			LOG(anwser);
			process_incoming(anwser + ";");
		}
		//LOG(buffer);

		return 1;
	}

	bool endsWith(const std::string &mainStr, const std::string &toMatch)
	{
		if (mainStr.size() >= toMatch.size() &&
			mainStr.compare(mainStr.size() - toMatch.size(), toMatch.size(), toMatch) == 0)
			return true;
		else
			return false;
	}

public:
	// Constructor for SML usage. Do not put anything in here, use setup() instead.
	Communication() : Mod(modInfo) {
	}
	



	// The setup function is the heart of the mod, where you hook your functions and register your commands and API functions. Do not rename!
	void setup() override {
		// Use the placeholders namespace
		using namespace std::placeholders;

		SDK::InitSDK(); //Initialize the SDK in ExampleMod so the functions work properly

		// More on namespaces:
		// * The functions that will be of use to you are in the SML::Mods::Functions namespace. A tip is to type Functions:: and see what functions are available for you to use. 

		// Hook a member function as handler
		::subscribe<&AFGPlayerController::BeginPlay>(std::bind(&Communication::beginPlay, this, _1, _2)); //bind the beginPlay function, with placeholder variables
		// Because there are two inputs to the function, we use _1 and _2. If there were 3 inputs, we would use _1, _2, and _3, and so forth.

		// Hook a lambda with captured this-ptr as handler
		::subscribe<&PlayerInput::InputKey>([this](Functions::ModReturns* modReturns, PlayerInput* playerInput, FKey key, InputEvent event, float amount, bool gamePad) {

			if (GetAsyncKeyState('H')) {
				Functions::sendMessageToPlayer("hi\00");
			}

			if (GetAsyncKeyState('L')) {
				//sending_message("HELLLLLOoooooo!");
			}
			return false;
		});

		
		// Tick functions are called every frame of the game. BE VERY CAREFUL WHEN USING THIS FUNCTION!!! Putting a lot of code in here will slow the game down to a crawl!
		::subscribe<&UWorld::Tick>([this](Functions::ModReturns*, UWorld* world, ELevelTick tick, float delta) {
			// If you abuse this, I will find you, and I will... uh... do something... and you won't like it
			//LOG("test");
			
			std::string test = Functions::getWorld()->CurrentLevel->GetFullName();
			//LOG(test);

			//prevent crash when event is triggered during player is not inside the game!
			if (test == "Level Persistent_Level.Persistent_Level.PersistentLevel" || endsWith(test, "PreviewBuildingWorld.PersistentLevel")) {
				counter++;
				if (counter % 60 == 0) {
					listens();
				}
			}
		});

		
		
		//Register a custom event. This does not call the event, you have to do that with Functions::broadcastEvent.
		Functions::registerEvent("ExampleMod_PostSetupComplete", postSetupComplete);


		LOG("Finished Communication setup!");
	}

	//The postSetup function is where you do things based on other mods' setup functions
	void postSetup() override {
		// Write things to be done after other mods' setup functions
		// Called after the post setup functions of mods that you depend on.

	}

	~Communication() {
		// Cleanup
		LOG("ExampleMod finished cleanup!");
	}


};

// Required function to create the mod, do not rename!
MOD_API Mod* ModCreate() {
	return new Communication();
}