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
	int filled;

	Inventory() {}
	Inventory(int inv0, int inv1, int inv2, int inv3, int score)
		: Stones(inv0, inv1, inv2, inv3), score(score)
	{
		filled = blue + green + orange + yellow;
	}
};

// need declaration here for Action class method
Inventory inv;

struct Action : public Stones
{
	Action() {}
	Action(int blue, int green, int orange, int yellow)
		: Stones(blue, green, orange, yellow) {}

	virtual vector<int> getMissingStones()
	{
		vector<int> missing(4, 0);

		missing[0] = (blue < 0 && inv.blue < abs(blue)) ? abs(blue) - inv.blue : 0;
		missing[1] = (green < 0 && inv.green < abs(green)) ? abs(green) - inv.green : 0;
		missing[2] = (orange < 0 && inv.orange < abs(orange)) ? abs(orange) - inv.orange : 0;
		missing[3] = (yellow < 0 && inv.yellow < abs(yellow)) ? abs(yellow) - inv.yellow : 0;
		return missing;
	}

	virtual bool haveRequiredStones()
	{
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
	float weighted_cost;
	float profit;
	bool recursion_disabled = false;

	Recipe() {}
	Recipe(int delta0, int delta1, int delta2, int delta3, int price)
		: Action(delta0, delta1, delta2, delta3), price(price)
	{
		weighted_cost = blue_weight * abs(blue) + green_weight * abs(green) + orange_weight * abs(orange) + yellow_weight * abs(yellow);
		profit = (float)price / weighted_cost;
	}
};

struct Spell : public Action
{
	bool avail;
	bool repeat;
	bool recursion_disabled = false;
	float weighted_cost;
	float weighted_gain;
	float weighted;

	Spell() {}
	Spell(int delta0, int delta1, int delta2, int delta3, bool avail, bool repeat)
		: Action(delta0, delta1, delta2, delta3), avail(avail), repeat(repeat)
	{
		weighted_cost = 0;
		weighted_gain = 0;
		(blue <= 0) ? weighted_cost += blue_weight * abs(blue) : weighted_gain += blue_weight * blue;
		(green <= 0) ? weighted_cost += green_weight * abs(green) : weighted_gain += green_weight * green;
		(orange <= 0) ? weighted_cost += orange_weight * abs(orange) : weighted_gain += orange_weight * orange;
		(yellow <= 0) ? weighted_cost += yellow_weight * abs(yellow) : weighted_gain += yellow_weight * yellow;
		weighted = weighted_gain - weighted_cost;
	}
};

struct Tome : public Action
{
	int tome_index;
	int reward;
	float weighted_cost;
	float weighted_gain;
	float weighted;
	int distribution;
	bool freeloader = false;
	bool mutator = false;

	Tome() {}
	Tome(int delta0, int delta1, int delta2, int delta3, int tome_index, int reward)
		: Action(delta0, delta1, delta2, delta3), tome_index(tome_index), reward(reward)
	{
		weighted_cost = 0;
		weighted_gain = 0;
		(blue <= 0) ? weighted_cost += blue_weight * abs(blue) : weighted_gain += blue_weight * blue;
		(green <= 0) ? weighted_cost += green_weight * abs(green) : weighted_gain += green_weight * green;
		(orange <= 0) ? weighted_cost += orange_weight * abs(orange) : weighted_gain += orange_weight * orange;
		(yellow <= 0) ? weighted_cost += yellow_weight * abs(yellow) : weighted_gain += yellow_weight * yellow;
		weighted = weighted_gain - weighted_cost;

		{
			distribution = 0;

			if (blue > 0)
				distribution++;
			if (green > 0)
				distribution++;
			if (orange > 0)
				distribution++;
			if (yellow > 0)
				distribution++;
		}

		if (blue >= 0 && green >= 0 && orange >= 0 && yellow >= 0)
		{
			freeloader = true;
			return;
		}

		{
			int gain_count = 0;
			int cost_count = 0;

			if (blue > 0)
				gain_count++;
			if (blue < 0)
				cost_count++;

			if (green > 0)
				gain_count++;
			if (green < 0)
				cost_count++;

			if (orange > 0)
				gain_count++;
			if (orange < 0)
				cost_count++;

			if (yellow > 0)
				gain_count++;
			if (yellow < 0)
				cost_count++;

			if (gain_count == 1 && cost_count == 1)
				mutator = true;
		}
	}

	vector<int> getMissingStones()
	{
		vector<int> missing(4, 0);

		missing[0] = (inv.blue < tome_index) ? tome_index - inv.blue : 0;
		return missing;
	}

	bool haveRequiredStones()
	{
		if (inv.blue < tome_index)
			return false;
		return true;
	}

	int getDistribution()
	{
		int distribution = 0;

		if (blue > 0)
			distribution++;
		if (green > 0)
			distribution++;
		if (orange > 0)
			distribution++;
		if (yellow > 0)
			distribution++;

		return distribution;
	}
};

map<int, Recipe> recipes;
map<int, Tome> tomes;
map<int, Spell> spells;

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
		cerr << "spell " << x.first << " weighted_cost: " << x.second.weighted_cost << endl;
		cerr << "spell " << x.first << " weighted_gain: " << x.second.weighted_gain << endl;
		cerr << "spell " << x.first << " weighted: " << x.second.weighted << endl;
	}
	cerr << "-------------TOME-------------" << endl;
	for (auto x : tomes)
	{
		cerr << "tome " << x.first << " stones: [" << x.second.blue;
		cerr << ", " << x.second.green;
		cerr << ", " << x.second.orange;
		cerr << ", " << x.second.yellow << "]" << endl;
		cerr << "tome " << x.first << " weighted_cost: " << x.second.weighted_cost << endl;
		cerr << "tome " << x.first << " weighted_gain: " << x.second.weighted_gain << endl;
		cerr << "tome " << x.first << " weighted: " << x.second.weighted << endl;
		cerr << "tome " << x.first << " distribution: " << x.second.distribution << endl;
		cerr << "tome " << x.first << " freeloader: " << x.second.freeloader << endl;
		cerr << "tome " << x.first << " mutator: " << x.second.mutator << endl;
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
		cerr << "recipe " << x.first << " weighted_cost: " << x.second.weighted_cost << endl;
		cerr << "recipe " << x.first << " profit: " << x.second.profit << endl;
	}
	cerr << "-------------INVENTORY-------------" << endl;
	cerr << "blue: " << inv.blue << endl;
	cerr << "green: " << inv.green << endl;
	cerr << "orange: " << inv.orange << endl;
	cerr << "yellow: " << inv.yellow << endl;
	cerr << "score: " << inv.score << endl;
	cerr << "filled:" << inv.filled << endl;
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

int getHighestPriceReceipe()
{
	int max_price = 0;
	int ret;

	for (auto &recipe : recipes)
	{
		if (recipe.second.price > max_price)
		{
			max_price = recipe.second.price;
			ret = recipe.first;
		}
	}
	return ret;
}

int getOptimalProfitRecipe()
{
	float max = 0;
	int ret;

	for (auto &recipe : recipes)
	{
		if (recipe.second.profit > max)
		{
			max = recipe.second.profit;
			ret = recipe.first;
		}
	}
	return ret;
}

int getOptimalCompletedRecipe()
{
	int highest_price = 0;
	int ret = 0;

	for (auto &recipe : recipes)
	{
		if (recipe.second.haveRequiredStones() && recipe.second.price > highest_price)
		{
			ret = recipe.first;
			highest_price = recipe.second.price;
		}
	}
	if (ret == 0)
		return -1;
	return ret;
}

int getLowestCostRecipe()
{
	float lowest_cost = INT8_MAX;
	int ret = 0;

	for (auto &recipe : recipes)
	{
		if (recipe.second.weighted_cost < lowest_cost)
		{
			ret = recipe.first;
			lowest_cost = recipe.second.weighted_cost;
		}
	}
	return ret;
}

int getClosestToCompletionRecipe()
{
	int min_missing = INT8_MAX;
	int ret = -1;

	for (auto &recipe : recipes)
	{
		std::vector<int> missing = recipe.second.getMissingStones();
		int missing_stones = 0;

		if (missing[0] > 0)
			missing_stones += missing[0];
		if (missing[1] > 0)
			missing_stones += missing[1];
		if (missing[2] > 0)
			missing_stones += missing[2];
		if (missing[3] > 0)
			missing_stones += missing[3];

		if (missing_stones < min_missing && !recipe.second.recursion_disabled)
		{
			ret = recipe.first;
			min_missing = missing_stones;
		}
	}
	return ret;
}

////////////////////////////////////// SPELLS ///////////////////////////////////

void enableAllSpells()
{
	for (auto &spell : spells)
		spell.second.recursion_disabled = false;
}

bool willOverflow(int spell_id)
{
	int count = 0;

	count = spells[spell_id].blue + spells[spell_id].green + spells[spell_id].orange + spells[spell_id].yellow;

	if (inv.filled + count > 10)
		return true;

	return false;
}

int getOptimalSpell(vector<int> missing)
{
	vector<int> enabled_spells;
	vector<int> eligible_spells;
	vector<int> solving_spells;
	vector<int> not_overflowing_spells;
	int ret = -1;
	int highest_gain = 0;

	for (auto &spell : spells)
	{
		if (!spell.second.recursion_disabled)
			enabled_spells.push_back(spell.first);
	}

	for (auto i : enabled_spells)
	{
		if (!willOverflow(i))
			not_overflowing_spells.push_back(i);
	}

	for (auto i : not_overflowing_spells)
	{
		if (spells[i].blue > 0 && missing[0] > 0)
		{
			eligible_spells.push_back(i);
			continue;
		}
		if (spells[i].green > 0 && missing[1] > 0)
		{
			eligible_spells.push_back(i);
			continue;
		}
		if (spells[i].orange > 0 && missing[2] > 0)
		{
			eligible_spells.push_back(i);
			continue;
		}
		if (spells[i].yellow > 0 && missing[3] > 0)
		{
			eligible_spells.push_back(i);
			continue;
		}
	}

	for (auto i : eligible_spells)
	{
		int count = 0;

		count = (missing[0] != 0 && spells[i].blue < missing[0]) ? count : count + 1;
		count = (missing[1] != 0 && spells[i].green < missing[0]) ? count : count + 1;
		count = (missing[2] != 0 && spells[i].orange < missing[0]) ? count : count + 1;
		count = (missing[3] != 0 && spells[i].yellow < missing[0]) ? count : count + 1;

		if (count == 4)
			solving_spells.push_back(i);
	}

	cerr << "size of solving spells: " << solving_spells.size() << endl;

	if (!solving_spells.empty())
	{
		highest_gain = 0;

		for (auto i : solving_spells)
		{
			if (ret != -1)
			{
				if (spells[ret].haveRequiredStones() && !spells[i].haveRequiredStones())
					continue;
			}
			if (spells[i].weighted_gain >= highest_gain)
			{
				if (ret != -1)
				{
					if (spells[i].weighted_gain == highest_gain)
					{
						if (spells[i].weighted_cost > spells[ret].weighted_cost)
							continue;
					}
				}
				highest_gain = spells[i].weighted_gain;
				ret = i;
			}
		}
		return ret;
	}

	for (auto i : eligible_spells)
	{
		int gain = 0;

		if (ret != -1)
		{
			if (spells[ret].haveRequiredStones() && !spells[i].haveRequiredStones())
				continue;
		}

		if (spells[i].blue > 0 && missing[0] > 0)
			gain += spells[i].blue;
		if (spells[i].green > 0 && missing[1] > 0)
			gain += spells[i].green;
		if (spells[i].orange > 0 && missing[2] > 0)
			gain += spells[i].orange;
		if (spells[i].yellow > 0 && missing[3] > 0)
			gain += spells[i].yellow;

		if (gain >= highest_gain)
		{
			if (ret != -1)
			{
				if (gain == highest_gain)
				{
					if (spells[i].weighted_cost > spells[ret].weighted_cost)
						continue;
				}
			}
			highest_gain = gain;
			ret = i;
		}
	}
	return ret;
}

////////////////////////////////////// GENERAL ///////////////////////////////////

template <typename T>
void getRequiredStones(T action)
{
	int spell_id = getOptimalSpell(action.getMissingStones());

	if (spell_id < 0)
	{
		cerr << "no available spells for required stones at this moment" << endl;
		cerr << "will try to complete recipe" << endl;

		int recipe_id = getOptimalCompletedRecipe();
		if (recipe_id < 0)
		{
			cerr << "no completed recipe available" << endl;
			recipe_id = getClosestToCompletionRecipe();
			if (recipe_id < 0)
			{
				cerr << "not possible to complete any recipes" << endl;
				cout << "WAIT" << endl;
				return;
			}

			cerr << "trying to complete recipe: " << recipe_id << endl;
			cerr << "calling getRequiredStones() recursively" << endl;

			enableAllSpells();
			recipes[recipe_id].recursion_disabled = true;
			getRequiredStones(recipes[recipe_id]);
			return;
		}

		else
		{
			cerr << "brewing recipe " << recipe_id << endl;
			cout << "BREW " << recipe_id << endl;
			return;
		}
	}

	cerr << "optimal spell_id: " << spell_id << endl;

	if (!spells[spell_id].avail)
	{
		cout << "REST" << endl;
		return;
	}

	if ((spells[spell_id].haveRequiredStones()))
	{
		cerr << "casting spell: " << spell_id << endl;
		cout << "CAST " << spell_id << endl;
		return;
	}

	else
	{
		cerr << "calling getRequiredStones() recursively" << endl;
		spells[spell_id].recursion_disabled = true;
		getRequiredStones(spells[spell_id]);
	}
}

////////////////////////////////////// WEIGHTS ///////////////////////////////////

void updateWeights(int id)
{
	if (tomes[id].green > 0 && green_weight > 1)
	{
		green_weight = (tomes[id].freeloader) ? 1 : tomes[id].weighted_cost / tomes[id].green;
		cerr << "updated green weight: " << green_weight << endl;
	}
	if (tomes[id].orange > 0 && orange_weight > 1)
	{
		orange_weight = (tomes[id].freeloader) ? 1 : tomes[id].weighted_cost / tomes[id].orange;
		cerr << "updated orange weight: " << orange_weight << endl;
	}
	if (tomes[id].yellow > 0 && yellow_weight > 1)
	{
		yellow_weight = (tomes[id].freeloader) ? 1 : tomes[id].weighted_cost / tomes[id].yellow;
		cerr << "updated yellow weight: " << yellow_weight << endl;
	}
}

////////////////////////////////////// TOME ///////////////////////////////////

bool hasFreeloader()
{
	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 2)
			return true;
	}
	return false;
}

bool hasMutator()
{
	for (auto tome : tomes)
	{
		if (tome.second.mutator && tome.second.weighted >= 2)
			return true;
	}
	return false;
}

int getOptimalFreeloader()
{
	float highest_weight = 0;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : tomes)
	{
		if (tome.second.freeloader && tome.second.weighted >= 2)
			tmp_tomes.push_back(tome.first);
	}
	for (auto i : tmp_tomes)
	{
		if (tomes[i].weighted >= highest_weight)
		{
			if (ret != 0)
			{
				if (tomes[i].distribution > tomes[ret].distribution)
					continue;
			}
			ret = i;
			highest_weight = tomes[i].weighted;
		}
	}
	return ret;
}

int getOptimalMutator()
{
	float highest_weight = 0;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : tomes)
	{
		if (tome.second.mutator && tome.second.weighted >= 2)
			tmp_tomes.push_back(tome.first);
	}
	for (auto i : tmp_tomes)
	{
		if (tomes[i].weighted >= highest_weight)
		{
			if (ret != 0)
			{
				if (tomes[i].distribution > tomes[ret].distribution)
					continue;
			}
			ret = i;
			highest_weight = tomes[i].weighted;
		}
	}
	return ret;
}

int getOptimalTome()
{
	if (hasFreeloader())
		return getOptimalFreeloader();
	else if (hasMutator())
		return getOptimalMutator();
	else
		return -1;
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
	if (hasFreeloader() || hasMutator())
	{
		int tome_id = getOptimalTome();
		cerr << "target tome: " << tome_id << endl;
		getSpellFromTome(tome_id);
	}

	else if (recipes[getOptimalProfitRecipe()].haveRequiredStones())
	{
		int recipe_id = getOptimalProfitRecipe();

		cerr << "target recipe: " << recipe_id << endl;
		cerr << "brewing recipe " << recipe_id << " instead" << endl;
		cout << "BREW " << recipe_id << endl;
	}

	else
	{
		int recipe_id = getOptimalProfitRecipe();

		cerr << "target recipe: " << recipe_id << endl;
		cerr << "not enough stones for recipe: " << recipe_id << endl;
		getRequiredStones(recipes[recipe_id]);
	}
}

int main()
{
	while (true)
	{
		processInput();
		computeOutput();
		//printData();
	}
}