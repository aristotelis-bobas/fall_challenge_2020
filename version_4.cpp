

/// OPTIMIZATIONS /// thnx Stan lol

#pragma GCC optimize("Ofast")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#pragma GCC option("arch=native", "tune=native", "no-zero-upper")
#pragma GCC target "bmi2"

/////////////////////////////////////

#include <iostream>
#include <string>
#include <map>
#include <list>
#include <vector>
#include <chrono>
#include <cmath>

#define INITIAL_DEPTH_LIMIT 15
#define NODE_REST 777

#define MUTATOR_COST_DEPTH 2
#define MUTATOR_COST_SPREAD 1
#define MUTATOR_GAIN_SPREAD 1

#define TIER_1_VALUE 1
#define TIER_2_VALUE 2
#define TIER_3_VALUE 3
#define TIER_4_VALUE 4

using namespace std;
using namespace std::chrono;

////////////////////////////////////// CLASSES ///////////////////////////////////

class Benchmark
{
public:
	Benchmark() {}

	void startBenchmark() { start = high_resolution_clock::now(); }
	void endBenchmark()
	{
		end = high_resolution_clock::now();
		time_span = duration<double, std::milli>(end - start);
	}
	auto getResult() { return time_span.count(); }
	void printResult() { cerr << "benchmark took " << time_span.count() << " ms" << endl; }

private:
	high_resolution_clock::time_point start;
	high_resolution_clock::time_point end;
	duration<double, milli> time_span;
};

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

	virtual vector<int> getMissingStones(const Inventory &inv)
	{
		vector<int> missing(4, 0);

		for (int i = 0; i < 4; i++)
			missing[i] = (stones[i] < 0 && inv.stones[i] < abs(stones[i])) ? abs(stones[i]) - inv.stones[i] : 0;

		return missing;
	}

	virtual bool haveRequiredStones(const Inventory &inv)
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
		: Action(blue, green, orange, yellow, id), price(price) {}
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
	}

	vector<int> getMissingStones(const Inventory &inv)
	{
		vector<int> missing(4, 0);

		missing[0] = (inv.stones[0] < tax_cost) ? tax_cost - inv.stones[0] : 0;
		return missing;
	}

	bool haveRequiredStones(const Inventory &inv)
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
	int tier_based_cost;
	int tier_based_gain;
	int tier_based_nett;

	Spell() {}
	Spell(int blue, int green, int orange, int yellow, int id, bool avail, bool repeat)
		: Action(blue, green, orange, yellow, id), avail(avail), repeat(repeat)
	{
		tier_based_cost = 0;
		tier_based_gain = 0;

		(stones[0] <= 0) ? tier_based_cost += TIER_1_VALUE * abs(stones[0]) : tier_based_gain += TIER_1_VALUE * stones[0];
		(stones[1] <= 0) ? tier_based_cost += TIER_2_VALUE * abs(stones[1]) : tier_based_gain += TIER_2_VALUE * stones[1];
		(stones[2] <= 0) ? tier_based_cost += TIER_3_VALUE * abs(stones[2]) : tier_based_gain += TIER_3_VALUE * stones[2];
		(stones[3] <= 0) ? tier_based_cost += TIER_4_VALUE * abs(stones[3]) : tier_based_gain += TIER_4_VALUE * stones[3];

		tier_based_nett = tier_based_gain - tier_based_cost;
	}

	bool willOverflowInventory(const Inventory &inv, int mult = 1)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += (stones[i] * mult);
		if (inv.slots_filled + count > 10)
			return true;
		return false;
	}

	bool willOverflowInventory(const array<int, 5> &inv, int mult = 1)
	{
		int count = 0;

		for (int i = 0; i < 4; i++)
			count += (stones[i] * mult);
		if (inv[4] + count > 10)
			return true;
		return false;
	}

	bool haveRequiredStones(const Inventory &inv, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv.stones[i] < mult * abs(stones[i]))
				return false;
		}
		return true;
	}

	bool haveRequiredStones(const array<int, 5> &inv, int mult = 1)
	{
		for (int i = 0; i < 4; i++)
		{
			if (stones[i] < 0 && inv[i] < mult * abs(stones[i]))
				return false;
		}
		return true;
	}
};

////////////////////////////////////// GLOBALS //////////////////////////////////////

map<int, Recipe> g_recipes;
map<int, Tome> g_tomes;
map<int, Spell> g_spells;
Inventory g_inv;
int enemy_score;

////////////////////////////////////// BRUTE FORCE BACKTRACER //////////////////////////////////////

/*
	this search tree algorithm brute forces all combinations
	it starts adjusting its own depth limit once a solution is found
*/

class Node
{

public:

	/// object state

	array<int, 5> inv;
	map<int, bool> spells;
	vector<int> log;
	int depth;
	vector<Node> nodes;
	bool discarded = false;
	bool solved = false;
	int option_count;
	vector<int> options;
	map<int, int> repeats;
	bool rest_option = false;

	/// class state
	static map<int, vector<int>> targets;
	static int limit;
	static int nodes_searched;
	static int target_hits;
	static int error_hits;
	static int limit_hits;

	// root_node constructor
	Node(Inventory start_inv, map<int, Spell> search_spells, map<int, vector<int>> search_targets)
	{
		// initialize class state
		Node::targets = search_targets;
		Node::limit = INITIAL_DEPTH_LIMIT;
		Node::nodes_searched = 0;
		Node::target_hits = 0;
		Node::limit_hits = 0;
		Node::error_hits = 0;
		
		// initialize object state
		inv = {start_inv.stones[0], start_inv.stones[1], start_inv.stones[2], start_inv.stones[3], start_inv.slots_filled};
		for (auto &spell : search_spells)
			spells[spell.first] = spell.second.avail;
		depth = 0;

		// // benchmarking search
		// Benchmark bm;
		// bm.startBenchmark();

		// core mechanics
		if (targetHit())
			return;
		getOptions();
		if (!errorHit())
			increaseDepth();

		// // benchmark results
		// bm.endBenchmark();
		// cerr << "Searched " << nodes_searched << " nodes in " << bm.getResult() << " ms at max depth " << Node::limit << " and found ";
		// cerr << target_hits << " target hits, " << limit_hits << " limit hits, " << error_hits << " error hits." << endl;
	}

	// child_node constructor
	Node(array<int, 5> inv, map<int, bool> spells, vector<int> log, int depth)
		: inv(inv), spells(spells), log(log), depth(depth)
	{
		Node::nodes_searched++;
		if (limitHit())
			return;
		if (targetHit())
			return;
		getOptions();
		if (!errorHit())
			increaseDepth();
	}

	void increaseDepth()
	{
		for (auto spell_id : options)
		{
			vector<int> mutated_log = log;
			mutated_log.emplace_back(spell_id);
			nodes.emplace_back(mutatedInventory(spell_id), mutatedSpells(spell_id), mutated_log, depth + 1);
		}
		for (auto spell : repeats)
		{
			vector<int> mutated_log = log;
			mutated_log.emplace_back(spell.first * spell.second);
			nodes.emplace_back(mutatedInventory(spell.first, spell.second), mutatedSpells(spell.first), mutated_log, depth + 1);
		}
		if (rest_option)
		{
			vector<int> mutated_log = log;
			mutated_log.push_back(NODE_REST);
			map<int, bool> mutated_spells = spells;
			mutateSpellAvailability(mutated_spells);
			nodes.emplace_back(inv, mutated_spells, mutated_log, depth + 1);
		}
	}

	void getOptions()
	{
		for (auto &spell : spells)
		{
			if (g_spells[spell.first].haveRequiredStones(inv) && !g_spells[spell.first].willOverflowInventory(inv) && spell.second)
				options.push_back(spell.first);
			if (g_spells[spell.first].repeat && spell.second)
			{
				for (int mult = 2; mult < 10; mult++)
				{
					if (g_spells[spell.first].haveRequiredStones(inv, mult) && !g_spells[spell.first].willOverflowInventory(inv, mult))
						repeats.insert({spell.first, mult});
					else
						break;
				}
			}
			if (!rest_option && !spell.second)
				rest_option = true;
		}
		option_count = options.size() + repeats.size();
		if (rest_option)
			option_count++;
	}

	// vector<int> getOptimalLog()
	// {
	// 	vector<vector<int>> logs = getLogs();
	// 	vector<vector<int>> possible_results;
	// 	vector<int> result;

	// 	// no solutions
	// 	if (target_hits == 0)
	// 		return result;

	// 	// only 1 solution
	// 	if (target_hits == 1)
	// 		return logs[0];

	// 	// get all best solutions
	// 	for (auto log : results)
	// 	{
	// 		if (log.size() == limit)
	// 			possible_results.emplace_back(log);
	// 	}

	// 	///// CONTINUE HERE
	// }

	vector<vector<int>> getLogs()
	{
		vector<vector<int>> logs;

		if (discarded)
			return logs;
		else if (solved)
		{
			logs.emplace_back(log);
			return logs;
		}
		else
		{
			for (auto &node : nodes)
			{
				vector<vector<int>> child_logs = node.getLogs();
				if (!child_logs.empty())
				{
					for (auto &log : child_logs)
						logs.emplace_back(log);
				}
			}
			return logs;
		}
	}

	bool errorHit()
	{
		if (option_count == 0)
		{
			discarded = true;
			error_hits++;
			return true;
		}
		return false;
	}

	bool limitHit()
	{
		if (depth > limit)
		{
			discarded = true;
			limit_hits++;
			return true;
		}
		return false;
	}

	bool targetHit()
	{
		for (auto target : targets)
		{		
			int match = 0;
			for (int i = 0; i < 4; i++)
			{
				if (inv[i] < target.second[i])
					break;
				else
					match++;
			}
			if (match == 4)
			{
				log.emplace_back(target.first);
				solved = true;
				break;
			}
		}
		if (!solved)
			return false;
		if (depth < limit)
			Node::limit = depth;
		target_hits++;
		return true;
	}

	array<int, 5> mutatedInventory(int spell_id, int mult = 1)
	{
		array<int, 5> tmp = inv;
		
		/// reset filled slots
		tmp[4] = 0;

		for (int i = 0; i < 4; i++)
		{
			(g_spells[spell_id].stones[i] <= 0) ? tmp[i] -= mult * abs(g_spells[spell_id].stones[i]) : tmp[i] += mult * g_spells[spell_id].stones[i];
			tmp[4] += tmp[i];
		}

		// cerr << "inventory mutation from: [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << inv.stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << "to [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << tmp.stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << "by spell [";
		// for (int i = 0; i < 4; i++)
		// {
		// 	cerr << spells[spell_id].stones[i];
		// 	(i == 3) ? cerr << "] " : cerr << ", ";
		// }
		// cerr << endl;

		return tmp;
	}

	map<int, bool> mutatedSpells(int spell_id)
	{
		map<int, bool> tmp = spells;
		tmp[spell_id] = false;
		return tmp;
	}

	void mutateSpellAvailability(map<int, bool> &spells)
	{
		for (auto &spell : spells)
			spell.second = true;
	}
};

// static initializations
map<int, vector<int>> Node::targets;
int Node::limit;
int Node::nodes_searched;
int Node::target_hits;
int Node::error_hits;
int Node::limit_hits;

////////////////////////////////////// RECIPE MECHANICS //////////////////////////////////////

void printLogs(vector<vector<int>> results)
{
	// cerr << results.size() << " logs for recipe " << recipe.id << " [";
	// for (int i = 0; i < 4; i++)
	// {
	// 	cerr << recipe.stones[i];
	// 	(i == 3) ? cerr << "] " : cerr << ", ";
	// }
	// cerr << "missing [";
	// vector<int> missing = recipe.getMissingStones(g_inv);
	// for (int i = 0; i < 4; i++)
	// {
	// 	cerr << missing[i];
	// 	(i == 3) ? cerr << "]" : cerr << ", ";
	// }
	// cerr << ":" << endl;

	for (auto log : results)
	{
		cerr << "Recipe " << log.back() << " in " << log.size() - 1 << " steps: ";
		for (auto step : log)
		{
			if (step != NODE_REST)
				cerr << step;
			else
				cerr << "REST";
			cerr << ", ";
		}
		cerr << endl;
	}
}

void benchmarkSearcher()
{
	map<int, vector<int>> search_targets;
	for (auto recipe : g_recipes)
		search_targets[recipe.first] = recipe.second.getMissingStones(g_inv);

	Benchmark bm;
	bm.startBenchmark();

	for (int i = 0; i < 50; i++)
	{
		Node root_node(g_inv, g_spells, search_targets);
		//vector<vector<int>> results = root_node.getLogs();
		//printLogs(results);
	}
	bm.endBenchmark();
	bm.printResult();
	cout << "mean search time: " << bm.getResult() / 50.0 << endl;
}

/////////////////////////////////// TEST INPUT ///////////////////////////////////

void testInput()
{
	g_recipes.insert({60, Recipe(0, 0, -5, 0, 60, 16)});
	g_recipes.insert({62, Recipe(0, -2, 0, -3, 62, 19)});
	g_recipes.insert({72, Recipe(0, -2, -2, -2, 72, 19)});
	g_recipes.insert({74, Recipe(-3, -1, -1, -1, 74, 14)});
	g_recipes.insert({75, Recipe(-1, -3, -1, -1, 75, 16)});

	g_spells.insert({78, Spell(2, 0, 0, 0, 78, true, false)});
	g_spells.insert({79, Spell(-1, 1, 0, 0, 79, true, false)});
	g_spells.insert({80, Spell(0, -1, 1, 0, 80, true, false)});
	g_spells.insert({81, Spell(0, 0, -1, 1, 81, true, false)});
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
	//printTomes();
	printSpells();
}

////////////////////////////////////// HIGH LEVEL CONTROL FLOW ///////////////////////////////////

int getFreeTome()
{
	for (auto tome : g_tomes)
	{
		if (tome.second.tax_cost == 0)
			return tome.first;
	}
	return -1;
}

void printAction()
{
	cout << "WAIT" << endl;
}

int main()
{
	while (true)
	{
		testInput();
		benchmarkSearcher();
		//processInput();
		//printData();
		//searchRecipes();
		//printAction();
		return 0;
	}
}