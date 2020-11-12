#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace std;

//////////////////////////////////////////////// DECLARATIONS AND DATA ///////////////////////////////////

float blue_weight = 0.5;
float green_weight = 1;
float orange_weight = 1;
float yellow_weight = 1;

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

struct Recipe : public Stones
{
	int price;
	float weighted;
	float profit;

	Recipe() {}
	Recipe(int delta0, int delta1, int delta2, int delta3, int price)
		: Stones(delta0, delta1, delta2, delta3), price(price)
	{
		weighted = blue_weight * abs(blue) + green_weight * abs(green) + orange_weight * abs(orange) + yellow_weight * abs(yellow);
		profit = (float)price / weighted;
	}
};

struct Spell : public Stones
{
	bool avail;
	bool repeat;
	float weighted;

	Spell() {}
	Spell(int delta0, int delta1, int delta2, int delta3, bool avail, bool repeat)
		: Stones(delta0, delta1, delta2, delta3), avail(avail), repeat(repeat)
	{
		weighted = (blue_weight * blue + green_weight * green + orange_weight * orange + yellow_weight * yellow);
	}
};

struct Tome : public Stones
{
	int tome_index;
	int reward;
	float weighted;
	bool freeloader = false;

	Tome() {}
	Tome(int delta0, int delta1, int delta2, int delta3, int tome_index, int reward)
		: Stones(delta0, delta1, delta2, delta3), tome_index(tome_index), reward(reward)
	{
		weighted = (blue_weight * blue + green_weight * green + orange_weight * orange + yellow_weight * yellow);
		if (blue >= 0 && green >= 0 && orange >= 0 && yellow >= 0)
			freeloader = true;
	}
};

Inventory inv;
map<int, Recipe> recipes;
map<int, Spell> spells;
map<int, Tome> tomes;
vector<int> cast_options;

//////////////////////////////////////////////// DEBUG ///////////////////////////////////

void printData()
{
	cerr << "-------------SPELLS-------------" << endl;
	for (auto x : spells)
	{
		cerr << "spell " << x.first << ": [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "spell " << x.first << " weighted: " << x.second.weighted << endl;
	}
	cerr << "-------------TOME-------------" << endl;
	for (auto x : tomes)
	{
		cerr << "tome " << x.first << ": [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "tome " << x.first << " weighted: " << x.second.weighted << endl;
		cerr << "tome " << x.first << " freeloader: " << x.second.freeloader << endl;
	}
	cerr << "-------------RECIPES-------------" << endl;
	for (auto x : recipes)
	{
		cerr << "recipe " << x.first << ": [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "recipe " << x.first << " price: " << x.second.price << endl;
		cerr << "recipe " << x.first << " weighted: " << x.second.weighted << endl;
		cerr << "recipe " << x.first << " profit: " << x.second.profit << endl;
	}
	cerr << "-------------INVENTORY-------------" << endl;
	cerr << "blue: " << inv.blue << endl;
	cerr << "green: " << inv.green << endl;
	cerr << "orange: " << inv.orange << endl;
	cerr << "yellow: " << inv.yellow << endl;
	cerr << "score: " << inv.score << endl;
}

//////////////////////////////////////////////// INPUT ///////////////////////////////////

void processActions()
{
	int actionCount;
	cin >> actionCount;

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
	int max_gain;

	for (auto spell : spells)
	{
		int count = 0;
		Spell tmp = spell.second;

		if (tmp.blue > 0 && missing[0] > 0)
			count += (blue_weight * tmp.blue);
		if (tmp.green > 0 && missing[1] > 0)
			count += (green_weight * tmp.green);
		if (tmp.orange > 0 && missing[2] > 0)
			count += (orange_weight * tmp.orange);
		if (tmp.yellow > 0 && missing[3] > 0)
			count += (yellow_weight * tmp.yellow);

		if (count >= max_gain)
		{
			if (count == max_gain && ret != 0)
			{
				if (tmp.weighted < spells[ret].weighted)
					continue;
			}
			max_gain = count;
			ret = spell.first;
		}
	}
	return ret;
}

////////////////////////////////////// GENERAL FUNCTIONS ///////////////////////////////////

template <typename Action>
bool haveRequiredStones(Action action)
{
	if (action.blue < 0 && inv.blue < abs(action.blue))
		return false;
	if (action.green < 0 && inv.green < abs(action.green))
		return false;
	if (action.orange < 0 && inv.orange < abs(action.orange))
		return false;
	if (action.yellow < 0 && inv.yellow < abs(action.yellow))
		return false;
	return true;
}

template <>
bool haveRequiredStones<Tome>(Tome tome)
{
	if (inv.blue < tome.tome_index)
		return false;
	return true;
}

vector<int> getMissingStones(Stones action)
{
	vector<int> missing(4, 0);

	missing[0] = (action.blue < 0 && inv.blue < abs(action.blue)) ? abs(action.blue) - inv.blue : 0;
	missing[1] = (action.green < 0 && inv.green < abs(action.green)) ? abs(action.green) - inv.green : 0;
	missing[2] = (action.orange < 0 && inv.orange < abs(action.orange)) ? abs(action.orange) - inv.orange : 0;
	missing[3] = (action.yellow < 0 && inv.yellow < abs(action.yellow)) ? abs(action.yellow) - inv.yellow : 0;

	return missing;
}

void getRequiredStones(Stones action)
{
	int spell_id = getOptimalSpell(getMissingStones(action));

	if (!spells[spell_id].avail)
		cout << "REST" << endl;
	else if (haveRequiredStones(spells[spell_id]))
		cout << "CAST " << spell_id << endl;
	else
		getRequiredStones(spells[spell_id]);
}

////////////////////////////////////// TOME LOGIC ///////////////////////////////////

void updateWeights(int id)
{
	if (!tomes[id].freeloader)
		return;
	if (tomes[id].blue > 0 && tomes[id].blue * blue_weight > 1)
		blue_weight = 1.0 / tomes[id].blue;
	if (tomes[id].green > 0 && tomes[id].green * green_weight > 1)
		green_weight = 1.0 / tomes[id].green;
	if (tomes[id].orange > 0 && tomes[id].orange * orange_weight > 1)
		orange_weight = 1.0 / tomes[id].orange;
	if (tomes[id].yellow > 0 && tomes[id].yellow * yellow_weight > 1)
		yellow_weight = 1.0 / tomes[id].yellow;
}

int getBestFreeloader()
{
	float highest_weight = 0;
	vector<int> tmp_tomes;
	int ret;

	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 1.5)
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

bool hasFreeloader()
{
	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 1.5)
			return true;
	}
	return false;
}

void getSpellFromTome(int id)
{
	if (haveRequiredStones(tomes[id]))
	{
		cout << "LEARN " << id << endl;
		updateWeights(id);
	}
	else
		getRequiredStones(tomes[id]);
}

void computeOutput()
{
	if (hasFreeloader())
		getSpellFromTome(getBestFreeloader());
	else if (haveRequiredStones(getOptimalRecipe()))
		cout << "BREW " << getOptimalRecipe() << endl;
	else
		getRequiredStones(recipes[getOptimalRecipe()]);
}

////////////////////////////////////// MAIN ///////////////////////////////////

int main()
{
	while (true)
	{
		processInput();
		computeOutput();
		printData();
	}
}