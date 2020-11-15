#include <iostream>
#include <string>
#include <map>
#include <vector>

#define CREATION_SIMULATION_DEPTH 3
#define PRICE_WEIGHT 3
#define MUTATOR_COST_SPREAD 1
#define MUTATOR_GAIN_SPREAD 1
#define MUTATOR_COST_DEPTH 3
#define MUTATOR_CUTOFF_RATE 0.33

using namespace std;

////////////////////////////////////// OBJECTS ///////////////////////////////////

struct Recursion
{
	static int level;

	static int getLevel() { return level; }
	static int setLevel(int level) { Recursion::level = level; }
	static int incLevel() { Recursion::level++; }
};

int Recursion::level = 0;

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
	int spread;
	float rate = 0;

	Recipe() {}
	Recipe(int blue, int green, int orange, int yellow, int price)
		: Action(blue, green, orange, yellow), price(price)
	{
		spread = 0;
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0)
				spread++;
		}
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
	float cost_weighted;
	float gain_weighted;
	float nett_weighted;
	bool repeat;
	bool freeloader = false;
	bool mutator = false;
	int mutator_focus;
	float mutator_rate;

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

		if (cost_spread <= MUTATOR_COST_SPREAD && gain_spread <= MUTATOR_GAIN_SPREAD && cost_depth <= MUTATOR_COST_DEPTH)
		{
			mutator = true;
			for (int i = 0; i < 4; i++)
			{
				if (stones[i] > 0)
					mutator_focus = i;
			}
		}
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
	int absolute_gain;
	int absolute_cost;
	bool disable_recursion = false;

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

void restoreSpellAvailability(map<int, Spell> &spells)
{
	for (auto &spell : spells)
		spell.second.avail = true;
}

void restoreSpellRecursion(map<int, Spell> &spells)
{
	for (auto &spell : spells)
		spell.second.disable_recursion = false;
}

class Simulation : public Stones
{
public:
	Inventory inv;
	map<int, Spell> spells;
	vector<int> log;
	bool exit = false;

	Simulation(vector<int> missing, Inventory inv, map<int, Spell> spells)
		: Stones(missing), inv(inv), spells(spells) {}

	bool stonesMissing()
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] > 0)
				return true;
		}
		return false;
	}
};

class GreedySim : public Simulation
{
public:
	vector<int> log;
	Recipe recipe;

	GreedySim(Inventory inv, map<int, Spell> spells, Recipe recipe)
		: Simulation(recipe.getMissingStones(inv), inv, spells), recipe(recipe)
	{
		// cerr << "simulation missing [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << stones[i];
		// 	(i == 3) ? cerr << "]" << endl : cerr << ", ";
		// }

		// cerr << "recipe [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << recipe.stones[i];
		// 	(i == 3) ? cerr << "]" << endl : cerr << ", ";
		// }
	}

	void updateMissing()
	{
		vector<int> new_missing = recipe.getMissingStones(inv);
		for (int i = 0; i < 4; i++)
			stones[i] = new_missing[i];

		// cerr << "simulation missing [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << stones[i];
		// 	(i == 3) ? cerr << "]" << endl : cerr << ", ";
		// }
	}

	void updateInv(int spell_id)
	{
		for (int i = 0; i < 4; i++)
			(spells[spell_id].stones[i] <= 0) ? inv.stones[i] -= abs(spells[spell_id].stones[i]) : inv.stones[i] += spells[spell_id].stones[i];

		inv.slots_filled = 0;
		for (int i = 0; i < 4; i++)
			inv.slots_filled += inv.stones[i];

		// cerr << "simulation inventory [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << inv.stones[i];
		// 	(i == 3) ? cerr << "]" : cerr << ", ";
		// }
		// cerr << " filled for [" << inv.slots_filled << "/10]" << endl;
	}

	int getSimulationSpell(vector<int> missing)
	{
		vector<int> eligible_spells;
		vector<int> productive_spells;
		int gather_max = -1;
		int ret = -1;

		// cerr << "------------------------" << endl;
		// for (auto spell : spells)
		// {
		// 	cerr << "spell " << spell.first << " [";
		// 	for (int i = 0; i < 4; i++)
		// 	{
		// 		cerr << spell.second.stones[i];
		// 		(i == 3) ? cerr << "]" : cerr << ", ";
		// 	}
		// 	cerr << " and is ";
		// 	if (!spell.second.avail)
		// 		cerr << "not ";
		// 	cerr << "available" << endl;
		// 	cerr << "recursion is " << spell.second.disable_recursion << endl;
		// }
		// cerr << "------------------------" << endl;

		for (auto spell : spells)
		{
			if (!spell.second.disable_recursion && !spell.second.willOverflowInventory(inv))
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

			for (int i = 0; i < 4; i++)
			{
				if (spells[spell_id].stones[i] > 0 && missing[i] > 0)
					gathered = (spells[spell_id].stones[i] > missing[i]) ? gathered + missing[i] : gathered + spells[spell_id].stones[i];
			}

			if (gathered > gather_max)
			{
				gather_max = gathered;
				ret = spell_id;
			}
		}
		return ret;
	}

	template <typename T>
	void simulateAcquiringStones(T action)
	{
		int spell_id = getSimulationSpell(action.getMissingStones(inv));

		// cerr << "at recursion level " << Recursion::getLevel() << " spell is " << spell_id << endl;

		if (spell_id < 0)
		{
			exit = true;
			return;
		}

		if (!spells[spell_id].avail)
		{
			// cerr << "simulation rests at step " << log.size() << endl;
			restoreSpellAvailability(spells);
			log.push_back(-1);
			return;
		}

		if (spells[spell_id].haveRequiredStones(inv))
		{
			// cerr << "simulation casts spell " << spell_id << " at step " << log.size() << endl;
			log.push_back(spell_id);
			spells[spell_id].avail = false;
			updateInv(spell_id);
			updateMissing();
			return;
		}

		spells[spell_id].disable_recursion = true;
		// Recursion::incLevel();
		// cerr << "entering recursion level " << Recursion::getLevel() << endl;
		simulateAcquiringStones(spells[spell_id]);
		return;
	}

	int simulate()
	{
		while (stonesMissing())
		{
			// Recursion::setLevel(0);
			restoreSpellRecursion(spells);
			simulateAcquiringStones(recipe);
			if (exit)
				return INT16_MAX;
		}

		return log.size();
	}
};

map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
vector<float> g_creation_rates(4, 0);

////////////////////////////////////// SPELL CASTING //////////////////////////////////////

int getSpell(vector<int> missing)
{
	vector<int> eligible_spells;
	vector<int> productive_spells;
	int gather_max = -1;
	int ret = -1;

	for (auto spell : g_spells)
	{
		if (!spell.second.disable_recursion && !spell.second.willOverflowInventory(g_inv))
			eligible_spells.push_back(spell.first);
	}

	for (auto spell_id : eligible_spells)
	{
		for (int i = 0; i < 4; i++)
		{
			if (g_spells[spell_id].stones[i] > 0 && missing[i] > 0)
			{
				productive_spells.push_back(spell_id);
				break;
			}
		}
	}

	for (auto spell_id : productive_spells)
	{
		int gathered = 0;

		for (int i = 0; i < 4; i++)
		{
			if (g_spells[spell_id].stones[i] > 0 && missing[i] > 0)
				gathered = (g_spells[spell_id].stones[i] > missing[i]) ? gathered + missing[i] : gathered + g_spells[spell_id].stones[i];
		}

		if (gathered > gather_max)
		{
			gather_max = gathered;
			ret = spell_id;
		}
	}
	return ret;
}

template <typename T>
void getRequiredStones(T action)
{
	int spell_id = getSpell(action.getMissingStones(g_inv));

	if (!g_spells[spell_id].avail)
	{
		cout << "REST" << endl;
		return;
	}

	if (g_spells[spell_id].haveRequiredStones(g_inv))
	{
		cout << "CAST " << spell_id << endl;
		return;
	}

	g_spells[spell_id].disable_recursion = true;
	getRequiredStones(g_spells[spell_id]);
	return;
}

////////////////////////////////////// CREATION RATE ///////////////////////////////////

int simulateCreationSteps(int blue, int green, int orange, int yellow, map<int, Spell> input_spells)
{
	Inventory sim_inv(0, 0, 0, 0, 0);
	Recipe sim_recipe(blue, green, orange, yellow, 0);
	map<int, Spell> sim_spells = input_spells;
	restoreSpellAvailability(sim_spells);

	GreedySim greedy_sim(sim_inv, sim_spells, sim_recipe);
	return greedy_sim.simulate();
}

float getCreationRate(int tier, map<int, Spell> sim_spells = g_spells)
{
	int delta = -1;
	float efficiency = 0;

	if (tier == 0)
	{
		for (int i = 0; i < CREATION_SIMULATION_DEPTH; i++, delta--)
			efficiency += (float)simulateCreationSteps(delta, 0, 0, 0, sim_spells) / abs(delta);
		return efficiency / 3.0;
	}
	else if (tier == 1)
	{
		for (int i = 0; i < CREATION_SIMULATION_DEPTH; i++, delta--)
			efficiency += (float)simulateCreationSteps(0, delta, 0, 0, sim_spells) / abs(delta);
		return efficiency / 3.0;
	}
	else if (tier == 2)
	{
		for (int i = 0; i < CREATION_SIMULATION_DEPTH; i++, delta--)
			efficiency += (float)simulateCreationSteps(0, 0, delta, 0, sim_spells) / abs(delta);
		return efficiency / 3.0;
	}
	else
	{
		for (int i = 0; i < CREATION_SIMULATION_DEPTH; i++, delta--)
			efficiency += (float)simulateCreationSteps(0, 0, 0, delta, sim_spells) / abs(delta);
		return efficiency / 3.0;
	}
}

void computeCreationRate()
{
	for (int i = 0; i < 4; i++)
		g_creation_rates[i] = getCreationRate(i);
}

////////////////////////////////////// TOME MECHANICS ///////////////////////////////////

float computeMutatorRate(Tome mutator)
{
	int color = mutator.mutator_focus;
	map<int, Spell> tmp_spells(g_spells);
	tmp_spells.insert({1000, Spell(mutator, true, mutator.repeat)});

	return (g_creation_rates[color] - getCreationRate(color, tmp_spells)) / g_creation_rates[color];
}

void setWeights(Tome &tome)
{
	tome.cost_weighted = 0;
	tome.gain_weighted = 0;
	tome.nett_weighted = 0;

	for (int i = 0; i < 4; i++)
	{
		if (tome.stones[i] > 0)
			tome.gain_weighted += tome.stones[i] * g_creation_rates[i];
		if (tome.stones[i] < 0)
			tome.cost_weighted += abs(tome.stones[i]) * g_creation_rates[i];
	}
	tome.nett_weighted = tome.gain_weighted - tome.cost_weighted;
}

void computeTomeRates()
{
	for (auto &tome : g_tomes)
	{
		setWeights(tome.second);
		if (tome.second.mutator)
			tome.second.mutator_rate = computeMutatorRate(tome.second);
	}
}

int getOptimalMutator()
{
	float highest_rate = 0;
	float cutoff_rate = 0;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : g_tomes)
	{
		if (tome.second.mutator)
		{
			if (tome.second.mutator_rate >= MUTATOR_CUTOFF_RATE)
				tmp_tomes.push_back(tome.first);
		}
	}
	for (auto i : tmp_tomes)
	{
		if (g_tomes[i].mutator_rate > highest_rate)
		{
			ret = i;
			highest_rate = g_tomes[i].mutator_rate;
		}
	}
	return ret;
}

int getOptimalFreeloader()
{
	float highest_gain = 0;
	vector<int> tmp_tomes;
	int ret = 0;

	for (auto tome : g_tomes)
	{
		if (tome.second.freeloader)
			tmp_tomes.push_back(tome.first);
	}
	for (auto i : tmp_tomes)
	{
		if (g_tomes[i].nett_weighted > highest_gain)
		{
			ret = i;
			highest_gain = g_tomes[i].nett_weighted;
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
		{
			if (tome.second.mutator_rate >= MUTATOR_CUTOFF_RATE)
				return true;
		}
	}
	return false;
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

////////////////////////////////////// RECIPE MECHANICS //////////////////////////////////////

int simulateRecipeSteps(int id)
{
	Inventory sim_inv(0, 0, 0, 0, 0);
	map<int, Spell> sim_spells(g_spells);

	restoreSpellAvailability(sim_spells);
	GreedySim sim(sim_inv, sim_spells, g_recipes[id]);

	return sim.simulate();
}

void computeRecipeRates()
{
	int id;
	int steps;

	for (auto &recipe : g_recipes)
	{
		id = recipe.first;
		steps = simulateRecipeSteps(id);
		recipe.second.rate = (PRICE_WEIGHT * g_recipes[id].price) / (float)steps;
		//cerr << "[GreedySim] recipe " << id << " -- " << steps << " steps -- " << recipe.second.price << " price -- " << recipe.second.rate << " rate " << endl;
	}
}

int getOptimalRecipe()
{
	float max = 0;
	int ret;

	for (auto recipe : g_recipes)
	{
		if (recipe.second.rate >= max)
		{
			if (recipe.second.rate == max)
			{
				if (recipe.second.price < g_recipes[ret].price)
					continue;
			}
			max = recipe.second.rate;
			ret = recipe.first;
		}
	}
	return ret;
}

////////////////////////////////////// CONTROL FLOW ///////////////////////////////////

void computeOutput()
{
	if (tomeAvailable())
	{
		int tome_id = getOptimalTome();
		cerr << "focusing on learning tome " << tome_id << endl;

		if (g_tomes[tome_id].haveRequiredStones(g_inv))
			cout << "LEARN " << tome_id << endl;
		else
		{
			cerr << "not enough stones to learn tome " << tome_id << endl;
			getRequiredStones(g_tomes[tome_id]);
		}
	}

	else
	{
		int recipe_id = getOptimalRecipe();
		cerr << "focusing on brewing recipe " << recipe_id << endl;

		if (g_recipes[recipe_id].haveRequiredStones(g_inv))
			cout << "BREW " << recipe_id << endl;
		else
		{
			cerr << "not enough stones to brew recipe " << recipe_id << endl;
			getRequiredStones(g_recipes[recipe_id]);
		}
	}
}

//////////////////////////////////////////////// DEBUG ///////////////////////////////////

void printStones()
{
	cerr << "-------------CREATION-------------" << endl;
	cerr << "creation efficiency [";
	for (int i = 0; i < 4; i++)
	{
		cerr << g_creation_rates[i];
		(i == 3) ? cerr << "]" << endl : cerr << ", ";
	}
}

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
			cerr << " - mutator - rate " << tome.second.mutator_rate;
		cerr << " - nett_weight - " << tome.second.nett_weighted << endl;
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
		cerr << " with rate " << recipe.second.rate << " for " << recipe.second.price << " rupees" << endl;
	}
}

void printInventory()
{
	cerr << "-------------INVENTORY-------------" << endl;
	cerr << "inventory [";
	for (int i = 0; i < 4; i++)
	{
		cerr << g_inv.stones[i];
		(i == 3) ? cerr << "]" : cerr << ", ";
	}
	cerr << " filled for [" << g_inv.slots_filled << "/10]" << endl;
}

void printData()
{
	printStones();
	printTomes();
	//printRecipes();
	// printSpells();
	// printInventory();
}

//////////////////////////////////////////////// GAME STATE ///////////////////////////////////

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
	}
}

void processInput()
{
	g_recipes.clear();
	g_spells.clear();
	g_tomes.clear();

	processActions();
	processInventory();
	computeCreationRate();
	computeTomeRates();
	computeRecipeRates();
}

int main()
{
	while (true)
	{
		processInput();
		printData();
		computeOutput();
	}
}
