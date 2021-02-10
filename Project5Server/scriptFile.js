
/**
 * This is a function that accepts a cumulus cloud's parameters such as:
 *
 * x - the x value of the cloud's current position
 * y - the y value of the cloud's current position
 * direction - an int, indicating the direction of the cloud (0 - right, 1 - left)
 * xstart - the cloud's predetermined left boundary (x value)
 * ystart - the cloud's predetermined left boundary (y value)
 * xend = the cloud's predetermined right boundary (x value)
 * yend - the cloud's predetermined right boundary (y value)
 *   
 * And, updates the coordinates and direction of the given cloud appropriately.
 */
function updateCCloud(id, x, y, direction, xstart,  ystart,  xend, yend)
{
	var min = x - 1;
	var max = x + 1;
	// direction is moving right 
	if (direction == 0) {
		if ( max >= xend) {
			direction = 1;
			setCumulusCloudDirection(id, 1);
		} else {
			x+=2;
		}
	// direction is moving left	
	} else {
		if ( min <= xstart) {
			direction = 0;
			setCumulusCloudDirection(id, 0);
		} else {
			x-=2;
		}
	
	}
	setCumulusCloudCoords(id, x, y);
	return "Update Culumbus Position for Id " + id + ".  X is " + x +  "  & Y is " + y + ".";
}

