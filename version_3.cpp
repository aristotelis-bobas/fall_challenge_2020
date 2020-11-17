
#pragma GCC optimize("O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops") //Optimization flags
#pragma GCC optimize("Ofast")
#pragma GCC option("arch=native","tune=native","no-zero-upper") //Enable AVX
#pragma GCC target("avx")  //Enable AVX
#pragma GCC target "bmi2"

#include <x86intrin.h> //AVX/SSE Extensions
#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <cmath>

#define SIMULATION_MAX_DEPTH 30

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
	string type;
	int id;

	Action() {}
	Action(int blue, int green, int orange, int yellow, int id)
		: Stones(blue, green, orange, yellow), id(id) {}

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
	int steps = 0;
	float rating = 0;
	list<string> instructions;

	Recipe() {}
	Recipe(int blue, int green, int orange, int yellow, int id, int price)
		: Action(blue, green, orange, yellow, id), price(price)
	{
		type = "recipe";
	}
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
	Tome(int blue, int green, int orange, int yellow, int id, int tax_cost, int tax_gain, bool repeat)
		: Action(blue, green, orange, yellow, id), tax_cost(tax_cost), tax_gain(tax_gain), repeat(repeat)
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

		type = "tome";
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
	Spell(int blue, int green, int orange, int yellow, int id, bool avail, bool repeat)
		: Action(blue, green, orange, yellow, id), avail(avail), repeat(repeat)
	{
		type = "spell";
	}

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

struct SimulationResult
{
	int id;
	int repeat;
	int size;
	list<string> instructions;

	SimulationResult(int id, int repeat, list<string> instructions)
		: id(id), repeat(repeat), instructions(instructions), size(instructions.size()) {}
};

////////////////////////////////////// SIMULATION  //////////////////////////////////////

class Simulation : public Stones
{
public:
	Inventory inv;
	map<int, Spell> spells;
	list<string> log;
	Action action;
	int depth;
	bool error = false;

	Simulation(Inventory inv, map<int, Spell> spells, Action action, list<string> log, int depth)
		: Stones(action.getMissingStones(inv)), inv(inv), spells(spells), action(action), log(log), depth(depth) {}

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
					spell.second.stones[i] /= spell.second.repeat_value;
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

	int getCleaningSpell()
	{
		vector<int> available_spells;
		Inventory tmp;
		int min_inventory = 10;
		int cleaning_spell = -1;

		for (auto &spell : spells)
		{
			if (spell.second.haveRequiredStones(inv) && !spell.second.willOverflowInventory(inv) && !spell.second.disable_recursion)
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

	int canRepeat(vector<int> missing, Spell spell)
	{
		int mult = 1;

		for (int i = 0; i < 4; i++)
		{
			if (spell.stones[i] > 0 && missing[i] > 0)
			{
				if (spell.stones[i] < missing[i] && spell.repeat)
				{
					if (ceil((float)missing[i] / spell.stones[i]) > mult)
						mult = ceil((float)missing[i] / spell.stones[i]);
				}
			}
		}

		while (mult > 1)
		{
			if (!Spell(spell.stones[0] * mult, spell.stones[1] * mult, spell.stones[2] * mult, spell.stones[3] * mult, spell.avail, spell.repeat, 999).willOverflowInventory(inv))
				return mult;
			mult--;
		}

		return 1;
	}

	void alterRepeatingSpell(int mult, Spell &spell)
	{

		for (int i = 0; i < 4; i++)
			spell.stones[i] = spell.stones[i] * mult;

		spell.repeat_flag = true;
		spell.repeat_value = mult;
	}

	vector<int> getSimulationSpells(vector<int> missing)
	{
		vector<int> simulation_spells;

		for (auto spell : spells)
		{
			if (!spell.second.disable_recursion && !spell.second.disable_simulation && !spell.second.willOverflowInventory(inv))
			{
				for (int i = 0; i < 4; i++)
				{
					if (spell.second.stones[i] > 0 && missing[i] > 0)
					{
						simulation_spells.push_back(spell.first);
						break;
					}
				}
			}
		}
		return simulation_spells;
	}

	vector<SimulationResult> getSimulationResults(vector<int> missing, vector<int> simulation_spells)
	{
		vector<SimulationResult> results;
		list<string> tmp_log;

		for (auto spell_id : simulation_spells)
		{
			int multi = canRepeat(missing, spells[spell_id]);

			if (multi > 1)
			{
				while (multi > 1)
				{
					map<int, Spell> tmp_spells = spells;
					alterRepeatingSpell(multi, tmp_spells[spell_id]);

					if (tmp_spells[spell_id].haveRequiredStones(inv))
						results.push_back(SimulationResult(spell_id, multi, log));
					else
					{
						Simulation sim(inv, tmp_spells, tmp_spells[spell_id], log, depth + 1);
						tmp_log = sim.getSimulationSteps();
						results.push_back(SimulationResult(spell_id, multi, tmp_log));
					}
				}
			}
			else
			{
				if (spells[spell_id].haveRequiredStones(inv))
					results.push_back(SimulationResult(spell_id, multi, log));
				else
				{
					Simulation sim(inv, spells, spells[spell_id], log, depth + 1);
					tmp_log = sim.getSimulationSteps();
					results.push_back(SimulationResult(spell_id, multi, tmp_log));
				}
			}
		}
		return results;
	}

	int getSimulationSpell(vector<int> missing)
	{
		/// get spells that actually help gain missing ingredients and are not disabled (recursion / simulation / overflow)

		vector<int> simulation_spells;
		simulation_spells = getSimulationSpells(missing);

		//// check whether if spells available for simulating

		if (simulation_spells.size() == 1 && !spells[simulation_spells.front()].repeat)
			return simulation_spells.front();

		/// run simulations

		vector<SimulationResult> results;
		results = getSimulationResults(missing, simulation_spells);

		/// filter simulation results that end in errors

		vector<SimulationResult> filtered_results;

		// cerr << "results: ";

		for (auto result : results)
		{
			if (result.instructions.back() != "error")
				filtered_results.push_back(result);

			// cerr << "[id " << result.id << ", " << result.size << "], ";
		}

		// cerr << endl;

		/// get return value (shortest simulation)

		int min_steps = INT8_MAX;
		int repeat_value = 0;
		bool repeat = false;
		int ret = -1;

		for (auto result : filtered_results)
		{
			if (result.size < min_steps)
			{
				min_steps = result.size;
				ret = result.id;

				if (result.repeat > 1)
				{
					repeat = true;
					repeat_value = result.repeat;
				}
				else
					repeat = false;
			}
		}

		if (repeat)
			alterRepeatingSpell(repeat_value, spells[ret]);

		return ret;
	}

	template <typename T>
	void takeSimulationStep(T action)
	{
		int spell_id = getSimulationSpell(action.getMissingStones(inv));

		if (spell_id < 0)
		{
			// if no spell is found most probably the inventory is full, try to clean inventory
			spell_id = getCleaningSpell();

			// if still no spell found the simulation stops here
			if (spell_id < 0)
				error = true;
			return;
		}

		if (spells[spell_id].haveRequiredStones(inv))
		{
			if (!spells[spell_id].avail)
			{
				restoreSpellAvailability(spells);
				log.push_back("REST");
				return;
			}

			if (spells[spell_id].repeat_flag)
				log.push_back("CAST " + to_string(spell_id) + " " + to_string(spells[spell_id].repeat_value));
			else
				log.push_back("CAST " + to_string(spell_id));
			spells[spell_id].avail = false;
			updateSimulationInventory(spell_id);
			updateSimulationMissing();
			return;
		}

		spells[spell_id].disable_recursion = true;
		takeSimulationStep(spells[spell_id]);
		return;
	}

	list<string> getSimulationSteps()
	{
		cerr << "entered simulation at depth " << depth << " for stones [";
		for (int i = 0; i < 4; i++)
		{
			cerr << stones[i];
			if (i != 3)
				cerr << ", ";
		}
		cerr << "]" << endl;

		if (depth > SIMULATION_MAX_DEPTH)
		{
			log.push_back("error");
			return log;
		}

		while (!action.haveRequiredStones(inv) && !error)
		{
			restoreSpellRecursion();
			restoreRepeatingSpells();
			takeSimulationStep(action);
			if (error)
			{
				log.push_back("error");
				return log;
			}
		}

		cerr << "instructions at depth " << depth << " : ";
		for (auto step : log)
			cerr << step << ", ";
		cerr << endl;

		return log;
	}
};

////////////////////////////////////// GLOBALS //////////////////////////////////////

map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
int enemy_score;
int potions_brewed = 0;

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

		Simulation sim(g_inv, g_spells, g_recipes[id], recipe.second.instructions, 0);
		recipe.second.instructions = sim.getSimulationSteps();
		recipe.second.steps = recipe.second.instructions.size();

		if (recipe.second.instructions.back() != "error")
			recipe.second.rating = pow(g_recipes[id].price, RECIPE_PRICE_WEIGHT) / pow(g_recipes[id].steps, RECIPE_STEPS_WEIGHT);
		else
			recipe.second.rating = -1;
	}
}

int getQuickestRecipe()
{
	int min_steps = INT8_MAX;
	int ret = -1;

	for (auto recipe : g_recipes)
	{
		if (recipe.second.steps <= min_steps)
		{
			if (ret != -1 && recipe.second.steps == min_steps)
			{
				if (recipe.second.rating < g_recipes[ret].rating)
					continue;
			}
			min_steps = recipe.second.steps;
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
			g_recipes.insert({id, Recipe(blue, green, orange, yellow, id, price)});
		else if (type == "CAST")
			g_spells.insert({id, Spell(blue, green, orange, yellow, id, castable, repeatable)});
		else if (type == "LEARN")
			g_tomes.insert({id, Tome(blue, green, orange, yellow, id, tome_index, tax_count, repeatable)});
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
	list<string> placeholder;
	list<string> instructions;

	if (tomeAvailable() && tomeLimits())
	{
		int tome_id = getOptimalTome();
		cerr << "TARGET: tome " << tome_id << endl;

		if (g_tomes[tome_id].haveRequiredStones(g_inv))
			cout << "LEARN " << tome_id << endl;
		else
		{
			Simulation sim(g_inv, g_spells, g_tomes[tome_id], placeholder, 0);
			instructions = sim.getSimulationSteps();
			cout << instructions.front() << endl;
		}
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
		{
			Simulation sim(g_inv, g_spells, g_recipes[recipe_id], placeholder, 0);
			instructions = sim.getSimulationSteps();
			cout << instructions.front() << endl;
		}
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
		cerr << " - rating " << recipe.second.rating << " - steps " << recipe.second.steps << " - " << recipe.second.price << " rupees" << endl;
	}
}

void printData()
{
	printRecipes();
	printTomes();
	//printSpells();
}
