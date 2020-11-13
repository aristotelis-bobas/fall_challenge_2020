#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace std;

//////////////////////////////////////////////// STRUCTS & GLOBALS ///////////////////////////////////

struct Stones
{
	int blue;
	int green;
	int orange;
	int yellow;

	Stones() {}
	Stones(int blue, int green, int orange, int yellow)
		: blue(blue), green(green), orange(orange), yellow(yellow) {}
};

struct Inventory : public Stones
{
	int score;

	Inventory() {}
	Inventory(int inv0, int inv1, int inv2, int inv3, int score)
		: Stones(inv0, inv1, inv2, inv3), score(score) {}
};

Inventory inv; /// need declaration here for Action class

struct Action : public Stones
{
	Action() {}
	Action(int blue, int green, int orange, int yellow)
		: Stones(blue, green, orange, yellow) {}

	virtual vector<int> getMissingStones()
	{
		vector<int> missing(4, 0);

		cerr << "Action getMissingStones() called" << endl;
		missing[0] = (blue < 0 && inv.blue < abs(blue)) ? abs(blue) - inv.blue : 0;
		missing[1] = (green < 0 && inv.green < abs(green)) ? abs(green) - inv.green : 0;
		missing[2] = (orange < 0 && inv.orange < abs(orange)) ? abs(orange) - inv.orange : 0;
		missing[3] = (yellow < 0 && inv.yellow < abs(yellow)) ? abs(yellow) - inv.yellow : 0;
		return missing;
	}

	virtual bool haveRequiredStones()
	{
		cerr << "Action haveRequiredStones() called" << endl;
		if (blue < 0 && inv.blue < abs(blue))
			return false;
		if (green < 0 && inv.green < abs(green))
			return false;
		if (orange < 0 && inv.orange < abs(orange))
			return false;
		if (yellow < 0 && inv.yellow < abs(yellow))
			return false;
		return true;
	}
};

float blue_weight = 1;
float green_weight = 2;
float orange_weight = 3;
float yellow_weight = 4;

struct Recipe : public Action
{
	int price;
	float weighted;
	float profit;

	Recipe() {}
	Recipe(int delta0, int delta1, int delta2, int delta3, int price)
		: Action(delta0, delta1, delta2, delta3), price(price)
	{
		weighted = blue_weight * abs(blue) + green_weight * abs(green) + orange_weight * abs(orange) + yellow_weight * abs(yellow);
		profit = (float)price / weighted;
	}
};

struct Spell : public Action
{
	bool avail;
	bool repeat;
	bool disable = false;
	float weighted;

	Spell() {}
	Spell(int delta0, int delta1, int delta2, int delta3, bool avail, bool repeat)
		: Action(delta0, delta1, delta2, delta3), avail(avail), repeat(repeat)
	{
		weighted = (blue_weight * blue + green_weight * green + orange_weight * orange + yellow_weight * yellow);
	}
};

struct Tome : public Action
{
	int tome_index;
	int reward;
	float weighted;
	bool freeloader = false;

	Tome() {}
	Tome(int delta0, int delta1, int delta2, int delta3, int tome_index, int reward)
		: Action(delta0, delta1, delta2, delta3), tome_index(tome_index), reward(reward)
	{
		weighted = (blue_weight * blue + green_weight * green + orange_weight * orange + yellow_weight * yellow);
		if (blue >= 0 && green >= 0 && orange >= 0 && yellow >= 0)
			freeloader = true;
	}

	vector<int> getMissingStones()
	{
		vector<int> missing(4, 0);

		cerr << "Tome getMissingStones() called" << endl;
		missing[0] = (inv.blue < tome_index) ? tome_index - inv.blue : 0;
		
		return missing;
	}

	bool haveRequiredStones()
	{
		cerr << "Tome haveRequiredStones() called" << endl;
		if (inv.blue < tome_index)
			return false;
		return true;
	}
};

map<int, Recipe> recipes;
map<int, Spell> spells;
map<int, Tome> tomes;

//////////////////////////////////////////////// DEBUG ///////////////////////////////////

void printData()
{
	cerr << "-------------WEIGHTS-------------" << endl;
	cerr << "blue weight: " << blue_weight << endl;
	cerr << "green weight: " << green_weight << endl;
	cerr << "orange weight: " << orange_weight << endl;
	cerr << "yellow weight: " << yellow_weight << endl;
	cerr << "-------------SPELLS-------------" << endl;
	for (auto x : spells)
	{
		cerr << "spell " << x.first << " stones: [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "spell " << x.first << " weighted: " << x.second.weighted << endl;
	}
	cerr << "-------------TOME-------------" << endl;
	for (auto x : tomes)
	{
		cerr << "tome " << x.first << " stones: [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "tome " << x.first << " weighted: " << x.second.weighted << endl;
		cerr << "tome " << x.first << " freeloader: " << x.second.freeloader << endl;
		cerr << "tome " << x.first << " index: " << x.second.tome_index << endl;
	}
	cerr << "-------------RECIPES-------------" << endl;
	for (auto x : recipes)
	{
		cerr << "recipe " << x.first << " stones: [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "recipe " << x.first << " price: " << x.second.price << endl;
		cerr << "recipe " << x.first << " weighted: " << x.second.weighted << endl;
		cerr << "recipe " << x.first << " profit: " << x.second.profit << endl;
	}
	// cerr << "-------------INVENTORY-------------" << endl;
	// cerr << "blue: " << inv.blue << endl;
	// cerr << "green: " << inv.green << endl;
	// cerr << "orange: " << inv.orange << endl;
	// cerr << "yellow: " << inv.yellow << endl;
	// cerr << "score: " << inv.score << endl;
}

//////////////////////////////////////////////// INPUT ///////////////////////////////////

void processActions()
{
	int actionCount;
	cin >> actionCount;
	cin.ignore();

	for (int i = 0; i < actionCount; i++)
	{
		int actionId;	   // the unique ID of this spell or recipe
		string actionType; // CAST, OPPONENT_CAST, LEARN, BREW
		int delta0;		   // tier-0 ingredient change
		int delta1;		   // tier-1 ingredient change
		int delta2;		   // tier-2 ingredient change
		int delta3;		   // tier-3 ingredient change
		int price;		   // the price in rupees if this is a potion
		bool castable;	   // 1 if this is a castable player spell
		int tomeIndex;	   // the index in the tome if this is a tome spell, equal to the read-ahead tax
		int taxCount;	   // the amount of taxed tier-0 ingredients you gain from learning this spell
		bool repeatable;   // 1 if this is a repeatable player spell

		cin >> actionId >> actionType >> delta0 >> delta1 >> delta2 >> delta3 >> price >> tomeIndex >> taxCount >> castable >> repeatable;
		cin.ignore();

		if (actionType == "BREW")
			recipes.insert({actionId, Recipe(delta0, delta1, delta2, delta3, price)});
		else if (actionType == "CAST")
			spells.insert({actionId, Spell(delta0, delta1, delta2, delta3, castable, repeatable)});
		else if (actionType == "LEARN")
			tomes.insert({actionId, Tome(delta0, delta1, delta2, delta3, tomeIndex, taxCount)});
	}
}

void processInventory()
{
	for (int i = 0; i < 2; i++)
	{
		int inv0;
		int inv1;
		int inv2;
		int inv3;
		int score;

		cin >> inv0 >> inv1 >> inv2 >> inv3 >> score;
		cin.ignore();

		if (i == 0)
			inv = Inventory(inv0, inv1, inv2, inv3, score);
	}
}

void processInput()
{
	recipes.clear();
	spells.clear();
	tomes.clear();

	processActions();
	processInventory();
}

////////////////////////////////////// RECIPES ///////////////////////////////////

int getOptimalRecipe()
{
	float max = 0;
	int ret;

	for (auto recipe : recipes)
	{
		if (recipe.second.profit > max)
		{
			max = recipe.second.profit;
			ret = recipe.first;
		}
	}
	return ret;
}

////////////////////////////////////// SPELLS ///////////////////////////////////

int getOptimalSpell(vector<int> missing)
{
	int ret = 0;
	int max_gain = 0;

	cerr << "missing[0]: " << missing[0] << endl;
	cerr << "missing[1]: " << missing[1] << endl;
	cerr << "missing[2]: " << missing[2] << endl;
	cerr << "missing[3]: " << missing[3] << endl;

	for (auto &spell : spells)
	{
		int count = 0;
		Spell tmp = spell.second;

		if (spell.second.blue > 0 && missing[0] > 0)
			count += spell.second.blue;
		if (spell.second.green > 0 && missing[1] > 0)
			count += spell.second.green;
		if (spell.second.orange > 0 && missing[2] > 0)
			count += spell.second.orange;
		if (spell.second.yellow > 0 && missing[3] > 0)
			count += spell.second.yellow;

		if (count > max_gain && !spell.second.disable)
		{
			max_gain = count;
			ret = spell.first;
		}
	}
	spells[ret].disable = true;
	return ret;
}

////////////////////////////////////// GENERAL FUNCTIONS ///////////////////////////////////

template <typename T>
void getRequiredStones(T action)
{
	int spell_id = getOptimalSpell(action.getMissingStones());

	cerr << "optimal spell_id: " << spell_id << endl;

	if (!spells[spell_id].avail) // optimize this?
		cout << "REST" << endl;
	else if ((spells[spell_id].haveRequiredStones()))
		cout << "CAST " << spell_id << endl;
	else
	{
		cerr << "calling getRequiredStones() recursively" << endl;
		getRequiredStones(spells[spell_id]);
	}
}

////////////////////////////////////// TOME LOGIC ///////////////////////////////////

void updateWeights(int id)
{
	if (tomes[id].blue > 0)
	{
		blue_weight = (tomes[id].blue * blue_weight) / tomes[id].weighted;
		cerr << "updated blue weight: " << blue_weight << endl;
	}
	if (tomes[id].green > 0)
	{
		green_weight = (tomes[id].green * green_weight) / tomes[id].weighted;
		cerr << "updated green weight: " << green_weight << endl;
	}
	if (tomes[id].orange > 0)
	{
		orange_weight = (tomes[id].orange * orange_weight) / tomes[id].weighted;
		cerr << "updated orange weight: " << orange_weight << endl;
	}
	if (tomes[id].yellow > 0)
	{
		yellow_weight = (tomes[id].yellow * yellow_weight) / tomes[id].weighted;
		cerr << "updated yellow weight: " << yellow_weight << endl;
	}
}

bool hasFreeloader()
{
	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 1)
			return true;
	}
	return false;
}

int getBestFreeloader()
{
	float highest_weight = 0;
	vector<int> tmp_tomes;
	int ret;

	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 1)
			tmp_tomes.push_back(tome.first);
	}
	for (auto i : tmp_tomes)
	{
		if (tomes[i].weighted > highest_weight)
		{
			ret = i;
			highest_weight = tomes[i].weighted;
		}
	}
	return ret;
}

bool hasGoodSpell()
{
	for (auto tome : tomes)
	{
		if (tome.second.weighted >= 2)
			return true;
	}
	return false;
}

int getBestSpell()
{
	float highest_weight = 0;
	int ret;

	for (auto tome : tomes)
	{
		if (tome.second.weighted > highest_weight)
		{
			ret = tome.first;
			highest_weight = tome.second.weighted;
		}
	}
	return ret;
}

void getSpellFromTome(int id)
{
	if (tomes[id].haveRequiredStones())
	{
		cout << "LEARN " << id << endl;
		updateWeights(id);
	}
	else
		getRequiredStones(tomes[id]);
}

////////////////////////////////////// CONTROL FLOW ///////////////////////////////////

void computeOutput()
{
	if (hasFreeloader())
	{
		cerr << "targeting freeloader: " << getBestFreeloader() << endl;
		getSpellFromTome(getBestFreeloader());
	}
	else if (hasGoodSpell())
	{
		cerr << "targeting tomespell: " << getBestSpell() << endl;
		getSpellFromTome(getBestSpell());
	}
	else if (recipes[getOptimalRecipe()].haveRequiredStones())
	{
		cerr << "brewing recipe: " << getOptimalRecipe() << endl;
		cout << "BREW " << getOptimalRecipe() << endl;
	}
	else
	{
		cerr << "acquiring stones for recipe: " << getOptimalRecipe() << endl;
		getRequiredStones(recipes[getOptimalRecipe()]);
	}
}

int main()
{
	while (true)
	{
		processInput();
		computeOutput();
		printData();
	}
}