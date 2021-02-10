/** 
 * A Key to the different Event Types:
 *
 * CHARACTER_COLLISION = 1, // When the character collides with anything
 * CHARACTER_JUMP = 2, // When character jumps
 * CHARACTER_FALL = 3, // When characters falls as a result of collision/floor absence
 * CHARACTER_DEATH = 4,// When the character dies 
 * CHARACTER_SPAWN = 5, // When the character spawns
 * CCLOUD_TRANSLATE = 6, // When the columbus cloud translates
 * SCLOUD_TRANSLATE = 7, // When the stratus cloud translates
 * USER_INPUT = 8, // When the user clicks something (&  its not related to recording)
 * START_REC = 9, // When the user clicks to  start recording
 * END_REC = 10, // When the user ends a recording
 * CLIENT_CONNECT = 11 // When the user attempts to end a recording
 */

 /**
  * Different Types Of Keyboard input:
  * 
  * ARROW = 0, A = 1, B = 2, C = 3, D = 4, E = 5, F = 6, G = 7, H = 8, I = 9, 
  * J = 10, K = 11, L = 12, M = 13, N = 14, O = 15, P = 16, Q = 17, R = 18, S = 19, 
  * T = 20, U = 21, V = 22, W = 23, X = 24, Y = 25, Z = 26, Shift = 27, Space = 28
  */

/**
 * Diffent Types of Actions:
 *
 * STILL = 0, FALL = 1, JUMP = 2, LEFT = 3, RIGHT = 4, UP = 5, DOWN = 6, PUSH = 7, DIE = 8, 
 * SCROLL_RIGHT = 9, SCROLL_LEFT = 10
 */

 /*
  * This is a function that reacts to user input. This includes creating raising events
  * based on the type of user input.
  * 
  * Keys are numerical. This is a key:
  * Key 0 = Right
  * Key 1 = Left 
  * Key 2 = Up
  */
function reactToKeyboard(key) {

	// Reacting To Right Key Press
	if (key == 0) {

		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		nums[0] = 0; // Arrow = 0
		nums[1] = 4; // Right = 4
		nums[2] = 0; // Vadility ?
		
		if (isRightValid() == true) {
		    nums[2] = 1;
		} else {

			var strings2 = [];
			strings2[0] = "action"; // the character collision action
			strings2[1] = "auto"; // auto = is this an automatic update?

			var nums2 = [];
			nums2[0] = getRightAction(); // TBD
			nums2[1] = 0; // false

			createRaiseEvent(1, 4, 2, strings2, nums2); // 1 = Character Collision
		}

		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 3, 3, strings, nums); // 8 = User Input 

	}

	// Reacting To Left Key Press
	if (key == 1) {
		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		nums[0] = 0; // Arrow = 0
		nums[1] = 3; // Left = 3
		nums[2] = 0; // Vadility ?
		
		if (isLeftValid() == true) {
		    nums[2] = 1;
		} else {

			var strings2 = [];
			strings2[0] = "action"; // the character collision action
			strings2[1] = "auto"; // auto = is this an automatic update?

			var nums2 = [];
			nums2[0] = getLeftAction(); // TBD
			nums2[1] = 0; // false

			createRaiseEvent(1, 4, 2, strings2, nums2); // 1 = Character Collision
		}

		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 3, 3, strings, nums); // 8 = User Input 
	}

	// Reacting To Up Key Press
	if (key == 2) {

		var strings = [];
		strings[0] = "key";
		strings[1] = "type";
		strings[2] = "valid";

		var nums = [];
		
		nums[0] = 0; // Arrow = 0
		nums[1] = 2; // Jump = 2
		nums[2] = 0; // Vadility ?
		
		if (isJumpValid() == true) {
		    nums[2] = 1;
			var empty1 = [];
			var empty2 = [];
			createRaiseEvent(2, 2, 0, empty1, empty2) // 2 = Character Jump
		} 

		// Create Raise Event Constructor = (int type, int priority, int args, std::vector<std::string> keys, std::vector<int> values);
		createRaiseEvent(8, 1, 3, strings, nums); // 8 = User Input 
	} 

	return "Success";
}

/**
 * Finds the value for the key in the 2D array.
 */
function findArgValue(arr, key) {
	for (var i = 0; i < arr.length; i++) {
		if (arr[i][0].localeCompare(key) == 0) {
			return arr[i][1];
		}
	}
}

/** 
 * This is a class that retrieves event data via a string and determines and 
 * acts on the worlds next move as a result. 
 */
function handleWorldEvent(message)
{
	// Get Array, Event Type & Num Of Arguments
	var eventArr = message.split(",");
	var type = eventArr[0]; // The Type of Event 
	var args = eventArr[3]; // The Number Of Arguments The Event Contains

	// A 2D Array of Arguments
	var argMap = [];
	var buff = 0;
	for (var i = 0; i < args; i++) {
		argMap[i] = [eventArr[4+buff], eventArr[6+buff]];
		buff += 3;
	}

	// If The Event Type Is CHARACTER COLLISION:
	if (parseInt(type) == 1) {

		// Get Action
		var action = findArgValue(argMap, "action");
		var numAction = parseInt(action);

		// Action UP = 5 -- Up is An Indication Of Avatar Colliding w/ Stratus Cloud
		if (numAction == 5) {
			worldStratusReact();
		}

		// Action SCROLL_RIGHT = 9 
		else if (numAction == 9) {
			worldScrollRight();
		}

		// Action SCROLL_LEFT = 10 
		else if (numAction == 10) {
			worldScrollLeft();
		}

		// If It's Another Action
		else {

			// Is it an Automatic Update?
			var auto = findArgValue(argMap, "auto");

			// And It's On Automatic Update; Update Action
			if (parseInt(auto) == 1) {
				worldUpdateAvatarJump(numAction);
			}
		}
	} 

	// If Event Type is CHARACTER DEATH:
	else if (parseInt(type) == 4) {
		worldKillAvatar();
	}

	// If Event Type is CHARACTER SPAWN
	else if (parseInt(type) == 5) {

		// Get Translate
		var trans = findArgValue(argMap, "translate");
		// var numTrans = pareInt(trans);

		// Update Avatar Jump Varaiable To STILL (0)
		worldUpdateAvatarJump(0);

		// Set Translate
		worldSetTranslate(trans);
	}  

	// If Event Type is CHARACTER FALL
	else if (parseInt(type) == 3) {

		// Update Avatar Jump Varaiable To FALL (1)
		worldUpdateAvatarJump(1);
	}

	// IF Event Type is CHARACTER JUMP
	else if (parseInt(type) == 2) {

		// Update Avatar Jump Varaiable To JUMP (2)
		worldUpdateAvatarJump(2);

	}

	return "Success";
}


