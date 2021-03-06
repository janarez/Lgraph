#include "LIntersect.h"


// constructor
// takes in graph in the form of map of vertices to its neighbours

LIntersectionGraph::LIntersectionGraph(size_t n)
{
	initClass(n);
}

void LIntersectionGraph::initClass(size_t n)
{
	// initializes all LShapes to zeroes;
	max = n;
	// 0 index unused
	shapes.resize(max + 1);

	// initializes partial order class
	cum.initializePartialOrder(max);
}

// -1 is left, 1 is right, 0 is undecide
void LIntersectionGraph::deduceDirections(void)
{
	// 0 index unused
	directions.assign(max+1,0);
	stops.resize(max + 1);
	std::iota(stops.begin(), stops.end(), 0);
	
	cum.zeroMatrix();

	// 1 points to right, max point to left for sure
	for (auto const& i : neighbors)
	{
		auto it = i.second.upper_bound(i.first);
		if (i.second.begin() == it)
		{
			directions[i.first] = 1;
		}
		else if (i.second.end() == it)
		{
			directions[i.first] = -1;
		}
	}
	return;
}

// this is a recursive functions that through direction guessing proceeds to find solution
// all possible directions are guessed and as such the complexity is O(2^N)
// first run with counter = 1 since dir[1] is set already
bool LIntersectionGraph::guessDirections(size_t counter)
{
	// marks valid directions configuration
	bool dirOK = true;
	// do guessing for all nonset directions
	for (size_t i = counter+1; i < max; ++i)
	{
		if (directions[i] == 0)
		{
			// all preceeding vertices are correct and so only higher vertices are checked
			auto it = neighbors[i].upper_bound(i);

			// first guess left
			directions[i] = -1;
			while (it != neighbors[i].end())
			{
				if (directions[*it] == 1)
					dirOK = false;
				//else
					//directions[*it] = -1;
				++it;
			}
			// next vertex and check if solution was found
			if (dirOK)
			{
				if (guessDirections(i))		// this is to get out of the recursion, solution must be in shapes at this time
					return true;
			}
			// then guess right
			directions[i] = 1;
			auto iter = neighbors[i].begin();
			while (*iter < i)
			{
				if (directions[*iter] == -1)
					dirOK = false;
				++iter;
			}
			if (dirOK)
			{
				if (guessDirections(i))
					return true;
			}
			else
			{
				directions[i] = 0;
				return false;
			}
		}
	}
	// look for solution
	return tryToFind();
}

bool LIntersectionGraph::tryToFind(void)
{
	// all directions must be set
	if (std::find(directions.begin()+1, directions.end(), 0) != directions.end())
		return false;

	// try to find the l graph
	deduceStopIntervals();
	// do CUM with known data
	return guessStopIntervals(1,0);
}

// the minimal value for the side coordinate
// correct directions are passed
void LIntersectionGraph::deduceStopIntervals(void)
{
	std::iota(stops.begin(), stops.end(), 0);
	for (size_t right = 1; right <= max; ++right)
	{
		for (auto left : neighbors[right])
		{
			// symetrical
			if (left > right)
				break;
			// right must reach at least to left
			if (directions[left] == -1 && directions[right] == -1)
			{
				stops[right] = left;
				break;
				// reach to min vertex
			}
			else if (directions[left] == 1 && directions[right] == 1)
			{
				stops[left] = right;
				// reach to max vertex
			}
		}
	}
	return;
}

bool LIntersectionGraph::guessStopIntervals(size_t counter, size_t neighbor)
{
	std::vector<size_t> prev(max+1);
	for (size_t right = counter; right <= max; ++right)
	{
		for (auto left : neighbors[right])
		{
			// symetrical
			if (left > right)
				break;
			// already handled
			if (right == counter && left <= neighbor)
				continue;
			// this part is guessing
			// necessarily: left is 1 right is -1

			// good
			if (stops[right] <= left || stops[left] >= right)
				continue;
			// guess: prolong left
			if (prev[left] == 0)
				prev[left] = stops[left];
			stops[left] = right;
			if (guessStopIntervals(right, left))
				return true;
			
			// guess: prolong right
			stops[left] = prev[left];
			if (prev[right] == 0)
				prev[right] = stops[right];
			stops[right] = left;
			if (guessStopIntervals(right, left))
				return true;			
			// error
			stops[right] = prev[right];
			return false;
		}
	}
	return doPartialOrder();
}

// creates cum from so far known data
// returns if cum exists
bool LIntersectionGraph::doPartialOrder(void)
{
	std::vector<int> allN(max);
	std::iota(allN.begin(), allN.end(), 1);

	bool cumIsOK = true;
	cum.zeroMatrix();

	for (size_t right = 1; right <= max; ++right)
	{
		for (auto left : allN)
		{
			// symmetrical
			if (left >= right)
				break;

			// edge exists
			if (neighbors[left].find(right) != neighbors[left].end())
			{
				if (directions[left] == -1 && directions[right] == -1)
				{
					// right is higher than left
					cumIsOK = cum.add(right, left);
				}
				else if (directions[left] == 1 && directions[right] == 1)
				{
					// right is lower than left
					cumIsOK = cum.add(left, right);
				}
				else if (stops[left] >= right)
				{
					// right is lower than left
					cumIsOK = cum.add(left, right);
				}
				else
				{
					// right is higher than left
					cumIsOK = cum.add(right, left);
				}
			}
			// no edge
			else
			{
				if (directions[left] == -1 && directions[right] == -1 && stops[right] <= left)
				{
					// right is lower than left
					cumIsOK = cum.add(left, right);
				}
				else if (directions[left] == 1 && directions[right] == 1 && stops[left] >= right)
				{
					// right is higher than left
					cumIsOK = cum.add(right, left);
				}
				else if (directions[left] == 1 && directions[right] == -1)
				{
					if (stops[left] >= right)
					{
						// right is higher than left
						cumIsOK = cum.add(right, left);
					}
					if (cumIsOK && stops[right] <= left)
					{
						// right is lower than left
						cumIsOK = cum.add(left, right);
					}
				}
			}
			if (!cumIsOK)
				return false;
		}
	}

	// assign bends from cum
	bends = cum.createOrdering();

	// check that there wasn't a cycle in cum
	if (bends.empty())
		return false;
	else
		return true;
}

// shapes are indexed from zero
void LIntersectionGraph::fillShapes(void)
{
	for (size_t i = 0; i < max; ++i)
	{
		shapes[i].setUp(i + 1);
		// the plus one is for leading vertical line in the graphical representation
		size_t vertexToBend = bends[i]; 
		// -1 since shapes are indexed from 0
		shapes[vertexToBend-1].setBend(i + 1);	// actually result from cum
		shapes[i].setSide(stops[i + 1]);
	}
	return;
}

bool LIntersectionGraph::createLGraph(void)
{
	deduceDirections();  // known directions

	if (!guessDirections(1))
	{
		// no solution
		return false;

	}
	else
	{
		// solution exists, fill in lshapes
		fillShapes();
		return true;
	}
}

int LIntersectionGraph::returnDirection(size_t i) const
{
	return directions[i + 1];
}


