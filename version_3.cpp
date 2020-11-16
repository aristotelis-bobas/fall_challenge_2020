#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cmath>

#define RECIPE_PRICE_WEIGHT 2
#define RECIPE_STEPS_WEIGHT 2

#define MUTATOR_COST_DEPTH 2
#define MUTATOR_COST_SPREAD 1
#define MUTATOR_GAIN_SPREAD 1

using namespace std;

////////////////////////////////////// CLASSES ///////////////////////////////////

struct Stones
{
	int stones[4];

	Stones() {}
	Stones(vector<int> stones)
	{
		for (int i = 0; i < 4; i++)
			this->stones[i] = stones[i];
	}
	Stones(int blue, int green, int orange, int yellow)
	{
		stones[0] = blue;
		stones[1] = green;
		stones[2] = orange;
		stones[3] = yellow;
	}
};

struct Inventory : public Stones
{
	int score;
	int slots_filled;

	Inventory() {}
	Inventory(int blue, int green, int orange, int yellow, int score)
		: Stones(blue, green, orange, yellow), score(score)
	{
		slots_filled = stones[0] + stones[1] + stones[2] + stones[3];
	}
};

class Action : public Stones
{
public:
	Action() {}
	Action(int blue, int green, int orange, int yellow)
		: Stones(blue, green, orange, yellow) {}

	virtual vector<int> getMissingStones(Inventory &inv)
	{
		vector<int> missing(4, 0);

		for (int i = 0; i < 4; i++)
			missing[i] = (stones[i] < 0 && inv.stones[i] < abs(stones[i])) ? abs(stones[i]) - inv.stones[i] : 0;

		return missing;
	}

	virtual bool haveRequiredStones(Inventory &inv)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv.stones[i] < abs(stones[i]))
				return false;
		}
		return true;
	}
};

class Recipe : public Action
{
public:
	int price;
	int steps_needed = 0;
	float rating = 0;

	Recipe() {}
	Recipe(int blue, int green, int orange, int yellow, int price)
		: Action(blue, green, orange, yellow), price(price) {}
};

class Tome : public Action
{
public:
	int tax_cost;
	int tax_gain;
	int gain_spread;
	int cost_spread;
	int gain_depth;
	int cost_depth;
	bool repeat;
	bool freeloader = false;
	bool mutator = false;

	Tome() {}
	Tome(int blue, int green, int orange, int yellow, int tax_cost, int tax_gain, bool repeat)
		: Action(blue, green, orange, yellow), tax_cost(tax_cost), tax_gain(tax_gain), repeat(repeat)
	{
		gain_spread = 0;
		cost_spread = 0;
		cost_depth = 0;
		gain_depth = 0;

		for (int i = 0; i < 4; i++)
		{
			if (stones[i] > 0)
			{
				gain_spread++;
				gain_depth += stones[i];
			}
			if (stones[i] < 0)
			{
				cost_spread++;
				cost_depth += abs(stones[i]);
			}
		}

		if (cost_spread == 0)
			freeloader = true;

		if (cost_spread <= MUTATOR_COST_SPREAD && gain_spread <= MUTATOR_GAIN_SPREAD && cost_depth <= MUTATOR_COST_DEPTH && repeat)
			mutator = true;
	}

	vector<int> getMissingStones(Inventory &inv)
	{
		vector<int> missing(4, 0);

		missing[0] = (inv.stones[0] < tax_cost) ? tax_cost - inv.stones[0] : 0;
		return missing;
	}

	bool haveRequiredStones(Inventory &inv)
	{
		if (inv.stones[0] < tax_cost)
			return false;
		return true;
	}
};

class Spell : public Action
{
public:
	bool avail;
	bool repeat;
	bool repeat_flag = false;
	int repeat_value = 0;
	bool disable_recursion = false;
	bool disable_simulation = false;

	Spell() {}
	Spell(int blue, int green, int orange, int yellow, bool avail, bool repeat)
		: Action(blue, green, orange, yellow), avail(avail), repeat(repeat) {}

	Spell(Tome tome, bool avail, bool repeat)
		: Action(tome.stones[0], tome.stones[1], tome.stones[2], tome.stones[3]), avail(avail), repeat(repeat) {}

	bool willOverflowInventory(Inventory inv)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += stones[i];

		if (inv.slots_filled + count > 10)
			return true;

		return false;
	}
};

////////////////////////////////////// SIMULATION  //////////////////////////////////////

int getSpell(vector<int> missing, map<int, Spell> &spells, Inventory &inv);
int getCleaningSpell(map<int, Spell> &spells, Inventory &inv);

class Simulation : public Stones
{
public:
	Inventory inv;
	map<int, Spell> spells;
	vector<int> log;
	Action action;
	bool error = false;

	Simulation(Inventory inv, map<int, Spell> spells, Action action)
		: Stones(action.getMissingStones(inv)), inv(inv), spells(spells), action(action) {}

	static void restoreSpellAvailability(map<int, Spell> &spells)
	{
		for (auto &spell : spells)
			spell.second.avail = true;
	}

	void restoreSpellRecursion()
	{
		for (auto &spell : spells)
			spell.second.disable_recursion = false;
	}

	void restoreRepeatingSpells()
	{
		for (auto &spell : spells)
		{
			if (spell.second.repeat_flag)
			{
				spell.second.repeat_flag = false;
				for (int i = 0; i < 4; i++)
					spell.second.stones[i] *= spell.second.repeat_value;
				spell.second.repeat_value = 0;
			}
		}
	}

	void updateSimulationMissing()
	{
		vector<int> new_missing = action.getMissingStones(inv);
		for (int i = 0; i < 4; i++)
			stones[i] = new_missing[i];
	}

	void updateSimulationInventory(int spell_id)
	{
		inv.slots_filled = 0;

		for (int i = 0; i < 4; i++)
		{
			(spells[spell_id].stones[i] <= 0) ? inv.stones[i] -= abs(spells[spell_id].stones[i]) : inv.stones[i] += spells[spell_id].stones[i];
			inv.slots_filled += inv.stones[i];
		}
	}

	template <typename T>
	void simulateAcquiringStones(T action)
	{
		int spell_id = getSpell(action.getMissingStones(inv), spells, inv);
		
		if (spell_id < 0)
		{
			spell_id = getCleaningSpell(spells, inv);
			if (spell_id < 0)
				error = true;
			return;
		}

		if (!spells[spell_id].avail)
		{
			restoreSpellAvailability(spells);
			log.push_back(-1);
			return;
		}

		if (spells[spell_id].haveRequiredStones(inv))
		{
			log.push_back(spell_id);
			spells[spell_id].avail = false;
			updateSimulationInventory(spell_id);
			updateSimulationMissing();
			return;
		}

		spells[spell_id].disable_recursion = true;
		simulateAcquiringStones(spells[spell_id]);
		return;
	}

	int getSimulationSteps()
	{
		while (!action.haveRequiredStones(inv) && !error)
		{
			restoreSpellRecursion();
			simulateAcquiringStones(action);
			if (error)
				return 42; // this will not be fatal, I hope
		}
		return log.size();
	}
};

////////////////////////////////////// GLOBALS //////////////////////////////////////

map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
int enemy_score;
int potions_brewed = 0;

////////////////////////////////////// SPELL MECHANICS //////////////////////////////////////

int getFreeTome();

int getCleaningSpell(map<int, Spell> &spells, Inventory &inv)
{
	vector<int> available_spells;
	Inventory tmp;
	int min_inventory = 10;
	int cleaning_spell = -1;

	for (auto &spell : spells)
	{
		if (spell.second.haveRequiredStones(inv) && !spell.second.willOverflowInventory(inv))
			available_spells.push_back(spell.first);
	}

	for (auto spell_id : available_spells)
	{
		tmp = inv;
		tmp.slots_filled = 0;

		for (int i = 0; i < 4; i++)
		{
			(spells[spell_id].stones[i] <= 0) ? tmp.stones[i] -= abs(spells[spell_id].stones[i]) : tmp.stones[i] += spells[spell_id].stones[i];
			tmp.slots_filled += tmp.stones[i];
		}

		if (tmp.slots_filled < min_inventory)
		{
			min_inventory = tmp.slots_filled;
			cleaning_spell = spell_id;
		}
	}
	return cleaning_spell;
}

bool canRepeat(vector<int> missing, Spell &spell, Inventory &inv)
{
	for (int i = 0; i < 4; i++)
	{
		if (spell.stones[i] > 0 && missing[i] > 0)
		{
			if (spell.stones[i] < missing[i] && spell.repeat)
			{
				Spell tmp(spell);

				if (!Spell(tmp.stones[0] * 2, tmp.stones[1] * 2, tmp.stones[2] * 2, tmp.stones[3] * 2, tmp.avail, tmp.repeat).willOverflowInventory(inv))
					return true;
			}
		}
	}
	return false;
}

void alterRepeatingSpell(vector<int> missing, Spell &spell, Inventory &inv)
{
	bool complete = false;
	int repeat_count = 0;

	for (int i = 0; i < 4; i++)
	{
		if (spell.stones[i] > 0 && missing[i] > 0)
		{
			if (ceil((float)missing[i] / spell.stones[i]) > repeat_count)
				repeat_count = ceil((float)missing[i] / spell.stones[i]);
		}
	}

	for (int mult = repeat_count; mult > 1; mult--)
	{
		if (!Spell(spell.stones[0] * mult, spell.stones[1] * mult, spell.stones[2] * mult, spell.stones[3] * mult, spell.avail, spell.repeat).willOverflowInventory(inv))
		{
			for (int i = 0; i < 4; i++)
				spell.stones[i] = spell.stones[i] * mult;

			spell.repeat_flag = true;
			spell.repeat_value = mult;
			return;
		}
	}
}

int getSpell(vector<int> missing, map<int, Spell> &spells, Inventory &inv)
{
	vector<int> eligible_spells;
	vector<int> productive_spells;
	int gather_max = -1;
	int ret = -1;

	for (auto spell : spells)
	{
		if (!spell.second.disable_recursion && !spell.second.disable_simulation && !spell.second.willOverflowInventory(inv))
			eligible_spells.push_back(spell.first);
	}

	for (auto spell_id : eligible_spells)
	{
		for (int i = 0; i < 4; i++)
		{
			if (spells[spell_id].stones[i] > 0 && missing[i] > 0)
			{
				productive_spells.push_back(spell_id);
				break;
			}
		}
	}

	for (auto spell_id : productive_spells)
	{
		int gathered = 0;

		if (canRepeat(missing, spells[spell_id], inv))
			alterRepeatingSpell(missing, spells[spell_id], inv);

		for (int i = 0; i < 4; i++)
		{
			if (spells[spell_id].stones[i] > 0 && missing[i] > 0)
				gathered = (spells[spell_id].stones[i] > missing[i]) ? gathered + missing[i] : gathered + spells[spell_id].stones[i];
		}

		if (gathered >= gather_max)
		{
			if (ret != -1 && gathered == gather_max)
			{
				if (!spells[spell_id].haveRequiredStones(inv) && spells[ret].haveRequiredStones(inv))
					continue;

				spells[ret].disable_simulation = true;
				int ret_steps = Simulation(inv, spells, spells[ret]).getSimulationSteps();
				spells[ret].disable_simulation = false;

				spells[spell_id].disable_simulation = true;
				int spell_steps = Simulation(inv, spells, spells[spell_id]).getSimulationSteps();
				spells[spell_id].disable_simulation = false;
				if (spell_steps > ret_steps)
					continue;
			}
			gather_max = gathered;
			ret = spell_id;
		}
	}
	return ret;
}

template <typename T>
void getRequiredStones(T action)
{
	int spell_id = getSpell(action.getMissingStones(g_inv), g_spells, g_inv);

	if (spell_id < 0)
	{
		spell_id = getCleaningSpell(g_spells, g_inv);
		if (spell_id < 0)
		{
			cout << "LEARN " << getFreeTome() << endl;
			return;
		}
	}

	if (!g_spells[spell_id].avail)
	{
		cout << "REST" << endl;
		return;
	}

	if (g_spells[spell_id].haveRequiredStones(g_inv))
	{
		if (g_spells[spell_id].repeat_flag)
			cout << "CAST " << spell_id << " " << g_spells[spell_id].repeat_value << " DOUBLE SPANKINGS" << endl;
		else
			cout << "CAST " << spell_id << endl;
		return;
	}

	g_spells[spell_id].disable_recursion = true;
	getRequiredStones(g_spells[spell_id]);
	return;
}

////////////////////////////////////// TOME MECHANICS ///////////////////////////////////

int getOptimalMutator()
{
	int color_max = -1;
	vector<int> tmp_tomes;
	int ret = -1;

	for (auto tome : g_tomes)
	{
		if (tome.second.mutator)
			tmp_tomes.push_back(tome.first);
	}

	for (auto id : tmp_tomes)
	{
		int highest_color = -1;

		for (int i = 0; i < 4; i++)
		{
			if (g_tomes[id].stones[i] > 0)
				highest_color = i;
		}

		if (highest_color >= color_max)
		{
			if (ret != -1 && highest_color == color_max)
			{
				if (g_tomes[id].gain_depth < g_tomes[ret].gain_depth)
					continue;
			}
			ret = id;
			color_max = highest_color;
		}
	}
	return ret;
}

int getOptimalFreeloader()
{
	int color_max = -1;
	vector<int> tmp_tomes;
	int ret = -1;

	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader)
			tmp_tomes.push_back(tome.first);
	}

	for (auto id : tmp_tomes)
	{
		int highest_color = -1;

		for (int i = 0; i < 4; i++)
		{
			if (g_tomes[id].stones[i] > 0)
				highest_color = i;
		}

		if (highest_color >= color_max)
		{
			if (ret != -1 && highest_color == color_max)
			{
				if (g_tomes[id].gain_depth < g_tomes[ret].gain_depth)
					continue;
			}
			ret = id;
			color_max = highest_color;
		}
	}
	return ret;
}

bool freeloaderAvailable()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader)
			return true;
	}
	return false;
}

bool mutatorAvailable()
{
	float cutoff_rate = 0;

	for (auto tome : g_tomes)
	{
		if (tome.second.mutator)
			return true;
	}
	return false;
}

int getFreeTome()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.tax_cost == 0)
			return tome.first;
	}
	return -1;
}

int getOptimalTome()
{
	if (freeloaderAvailable())
		return getOptimalFreeloader();
	else if (mutatorAvailable())
		return getOptimalMutator();
	else
		return -1;
}

bool tomeAvailable()
{
	if (freeloaderAvailable() || mutatorAvailable())
		return true;
	return false;
}

////////////////////////////// RECIPE SIMULATIONS & MECHANICS //////////////////////////////////////

void simulateRecipes()
{
	int id;
	int steps;

	for (auto &recipe : g_recipes)
	{
		int id = recipe.first;

		//simulation based on empty inventory and non-exhausted spells //
		Inventory sim_inv(0, 0, 0, 0, 0);
		map<int, Spell> sim_spells(g_spells);
		Simulation::restoreSpellAvailability(sim_spells);

		steps = Simulation(sim_inv, sim_spells, g_recipes[id]).getSimulationSteps();
		recipe.second.steps_needed = steps;
		recipe.second.rating = pow(g_recipes[id].price, RECIPE_PRICE_WEIGHT) / pow((float)steps, RECIPE_STEPS_WEIGHT);
	}
}

int getQuickestRecipe()
{
	int min_steps = INT8_MAX;
	int ret = -1;

	for (auto recipe : g_recipes)
	{
		if (recipe.second.steps_needed <= min_steps)
		{
			if (ret != -1 && recipe.second.steps_needed == min_steps)
			{
				if (recipe.second.rating < g_recipes[ret].rating)
					continue;
			}
			min_steps = recipe.second.steps_needed;
			ret = recipe.first;
		}
	}
	return ret;
}

int getOptimalRecipe()
{
	float max = 0;
	int ret;

	for (auto recipe : g_recipes)
	{
		if (recipe.second.rating >= max)
		{
			if (ret != -1 && recipe.second.rating == max)
			{
				if (recipe.second.price < g_recipes[ret].price)
					continue;
			}
			max = recipe.second.rating;
			ret = recipe.first;
		}
	}
	return ret;
}

/////////////////////////////////// INPUT & INITIALIZATIONS ///////////////////////////////////

void processActions()
{
	int action_count;
	cin >> action_count;
	cin.ignore();

	for (int i = 0; i < action_count; i++)
	{
		int id;			 // the unique ID of this spell or recipe
		string type;	 // CAST, OPPONENT_CAST, LEARN, BREW
		int blue;		 // tier-0 ingredient change
		int green;		 // tier-1 ingredient change
		int orange;		 // tier-2 ingredient change
		int yellow;		 // tier-3 ingredient change
		int price;		 // the price in rupees if this is a potion
		bool castable;	 // 1 if this is a castable player spell
		int tome_index;	 // the index in the tome if this is a tome spell, equal to the read-ahead tax
		int tax_count;	 // the amount of taxed tier-0 ingredients you gain from learning this spell
		bool repeatable; // 1 if this is a repeatable player spell

		cin >> id >> type >> blue >> green >> orange >> yellow >> price >> tome_index >> tax_count >> castable >> repeatable;
		cin.ignore();

		if (type == "BREW")
			g_recipes.insert({id, Recipe(blue, green, orange, yellow, price)});
		else if (type == "CAST")
			g_spells.insert({id, Spell(blue, green, orange, yellow, castable, repeatable)});
		else if (type == "LEARN")
			g_tomes.insert({id, Tome(blue, green, orange, yellow, tome_index, tax_count, repeatable)});
	}
}

void processInventory()
{
	for (int i = 0; i < 2; i++)
	{
		int blue;
		int green;
		int orange;
		int yellow;
		int score;

		cin >> blue >> green >> orange >> yellow >> score;
		cin.ignore();

		if (i == 0)
			g_inv = Inventory(blue, green, orange, yellow, score);
		if (i == 1)
			enemy_score = score;
	}
}

void processInput()
{
	g_recipes.clear();
	g_spells.clear();
	g_tomes.clear();

	processActions();
	processInventory();
}

////////////////////////////////////// HIGH LEVEL CONTROL FLOW ///////////////////////////////////

void printData();

bool tomeLimits()
{
	if (g_spells.size() > 10)
		return false;
	if (g_inv.score > 30)
		return false;
	return true;
}

bool finishGame()
{
	if (g_inv.score >= enemy_score && potions_brewed >= 5)
		return true;
	if (g_inv.score >= enemy_score + 10 && potions_brewed >= 4)
		return true;
	if (g_inv.score >= enemy_score + 20 && potions_brewed >= 3)
		return true;
	return false;
}

void computeOutput()
{
	if (tomeAvailable() && tomeLimits())
	{
		int tome_id = getOptimalTome();
		cerr << "TARGET: tome " << tome_id << endl;

		if (g_tomes[tome_id].haveRequiredStones(g_inv))
			cout << "LEARN " << tome_id << endl;
		else
			getRequiredStones(g_tomes[tome_id]);
	}

	else
	{
		int recipe_id = (finishGame()) ? getQuickestRecipe() : getOptimalRecipe();
		cerr << "TARGET: recipe " << recipe_id << endl;

		if (g_recipes[recipe_id].haveRequiredStones(g_inv))
		{
			cout << "BREW " << recipe_id << " HERE'S YOUR SHIT, SIR" << endl;
			potions_brewed++;
		}
		else
			getRequiredStones(g_recipes[recipe_id]);
	}
}

int main()
{
	while (true)
	{
		processInput();
		simulateRecipes();
		printData();
		computeOutput();
	}
}

//////////////////////////////////////////////// DEBUG /////////////////////////////////////////

void printSpells()
{
	cerr << "-------------SPELLS-------------" << endl;
	for (auto spell : g_spells)
	{
		cerr << "spell " << spell.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << spell.second.stones[i];
			(i == 3) ? cerr << "]" << endl : cerr << ", ";
		}
	}
}

void printTomes()
{
	cerr << "-------------TOMES-------------" << endl;
	for (auto tome : g_tomes)
	{
		cerr << "tome " << tome.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << tome.second.stones[i];
			(i == 3) ? cerr << "]" : cerr << ", ";
		}
		if (tome.second.freeloader)
			cerr << " - freeloader";
		else if (tome.second.mutator)
			cerr << " - mutator";
		cerr << endl;
	}
}

void printRecipes()
{
	cerr << "-------------RECIPES-------------" << endl;
	for (auto recipe : g_recipes)
	{
		cerr << "recipe " << recipe.first << " [";
		for (int i = 0; i < 4; i++)
		{
			cerr << recipe.second.stones[i];
			(i == 3) ? cerr << "]" : cerr << ", ";
		}
		cerr << " - rating " << recipe.second.rating << " - steps " << recipe.second.steps_needed << " - " << recipe.second.price << " rupees" << endl;
	}
}

void printData()
{
	printRecipes();
	printTomes();
	//printSpells();
}
