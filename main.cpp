#include <iostream>
#include <string>
#include <map>
#include <vector>

using namespace std;

struct Inventory
{
	int blue;
	int green;
	int orange;
	int yellow;
	int score;

	Inventory() {}
	Inventory(int inv0, int inv1, int inv2, int inv3, int score)
		: blue(inv0), green(inv1), orange(inv2), yellow(inv3), score(score){};
};

struct Recipe
{
	int blue;
	int green;
	int orange;
	int yellow;
	int rupees;
	int weighted;
	float profit;

	Recipe() {}
	Recipe(int delta0, int delta1, int delta2, int delta3, int price)
		: blue(delta0), green(delta1), orange(delta2), yellow(delta3), rupees(price)
	{
		weighted = 1 * abs(blue) + 2 * abs(green) + 3 * abs(orange) + 4 * abs(yellow);
		profit = (float)price / weighted;
	}
};

struct Spell
{
	int blue;
	int green;
	int orange;
	int yellow;
	bool avail;

	Spell() {}
	Spell(int delta0, int delta1, int delta2, int delta3, bool avail)
		: blue(delta0), green(delta1), orange(delta2), yellow(delta3), avail(avail){};
};

Inventory inv;
map<int, Recipe> recipes;
map<int, Spell> spells;
vector<int> brew_options;
vector<int> cast_options;

void printData()
{
	cerr << "-------------SPELLS-------------" << endl;
	for (auto x : spells)
	{
		cerr << x.first << ".blue: " << x.second.blue << endl;
		cerr << x.first << ".green: " << x.second.green << endl;
		cerr << x.first << ".orange: " << x.second.orange << endl;
		cerr << x.first << ".yellow: " << x.second.yellow << endl;
	}
	cerr << "-------------RECIPES-------------" << endl;
	for (auto x : recipes)
	{
		cerr << x.first << ".blue: " << x.second.blue << endl;
		cerr << x.first << ".green: " << x.second.green << endl;
		cerr << x.first << ".orange: " << x.second.orange << endl;
		cerr << x.first << ".yellow: " << x.second.yellow << endl;
		cerr << x.first << ".rupees: " << x.second.rupees << endl;
		cerr << x.first << ".weighted: " << x.second.weighted << endl;
		cerr << x.first << ".profit: " << x.second.profit << endl;
	}
	cerr << "-------------INVENTORY-------------" << endl;
	cerr << "blue: " << inv.blue << endl;
	cerr << "green: " << inv.green << endl;
	cerr << "orange: " << inv.orange << endl;
	cerr << "yellow: " << inv.yellow << endl;
	cerr << "score: " << inv.score << endl;
}

void processActions()
{
	int actionCount; // the number of spells and recipes in play
	cin >> actionCount;

	for (int i = 0; i < actionCount; i++)
	{
		int actionId;	   // the unique ID of this spell or recipe
		string actionType; // in the first league: BREW; later: CAST, OPPONENT_CAST, LEARN, BREW
		int delta0;		   // tier-0 ingredient change
		int delta1;		   // tier-1 ingredient change
		int delta2;		   // tier-2 ingredient change
		int delta3;		   // tier-3 ingredient change
		int price;		   // the price in rupees if this is a potion
		bool castable;	   // in the first league: always 0; later: 1 if this is a castable player spell

		int tomeIndex;	 // in the first two leagues: always 0; later: the index in the tome if this is a tome spell, equal to the read-ahead tax
		int taxCount;	 // in the first two leagues: always 0; later: the amount of taxed tier-0 ingredients you gain from learning this spell
		bool repeatable; // for the first two leagues: always 0; later: 1 if this is a repeatable player spell

		cin >> actionId >> actionType >> delta0 >> delta1 >> delta2 >> delta3 >> price >> tomeIndex >> taxCount >> castable >> repeatable;

		if (actionType == "BREW" || "CAST")
		{
			if (actionType == "BREW")
				recipes.insert({actionId, Recipe(delta0, delta1, delta2, delta3, price)});
			else if (actionType == "CAST")
				spells.insert({actionId, Spell(delta0, delta1, delta2, delta3, castable)});
		}
	}
}

void processInventory()
{
	// i == 0 is your own inventory
	for (int i = 0; i < 2; i++)
	{
		int inv0; // tier-0 ingredients in inventory
		int inv1;
		int inv2;
		int inv3;
		int score; // amount of rupees
		cin >> inv0 >> inv1 >> inv2 >> inv3 >> score;
		if (i == 0)
			inv = Inventory(inv0, inv1, inv2, inv3, score);
	}
}

void processInput()
{
	recipes.clear();
	spells.clear();

	processActions();
	processInventory();
}

template <typename Action>
bool haveIngredients(Action action)
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

bool canBrew()
{
	brew_options.clear();

	for (auto recipe : recipes)
	{
		// if (recipe.second.profit < 0.9)   // select better profitibility later
		// 	continue;
		if (!haveIngredients(recipe.second))
			continue;
		brew_options.push_back(recipe.first);
	}
	if (brew_options.empty())
		return false;
	return true;
}

void brewPotion()
{
	int output_id;
	int max = 0;

	for (auto option : brew_options)
	{
		if (recipes[option].profit > max)
		{
			max = recipes[option].profit;
			output_id = option;
		}
	}
	cout << "BREW " << output_id << endl;
}

void castSpell()
{

	for (auto option : cast_options)
	{
		if (spells[option].yellow > 0)
		{
			cout << "CAST " << option << endl;
			return;
		}
	}
	for (auto option : cast_options)
	{
		if (spells[option].orange > 0)
		{
			cout << "CAST " << option << endl;
			return;
		}
	}
	for (auto option : cast_options)
	{
		if (spells[option].green > 0)
		{
			cout << "CAST " << option << endl;
			return;
		}
	}
	for (auto option : cast_options)
	{
		if (spells[option].blue > 0)
		{
			cout << "CAST " << option << endl;
			return;
		}
	}
}

bool canCast()
{
	cast_options.clear();

	for (auto spell : spells)
	{
		if (!spell.second.avail)
			continue;
		if (!haveIngredients(spell.second))
			continue;
		cast_options.push_back(spell.first);
	}
	if (cast_options.empty())
		return false;
	return true;
}

void computeOutput()
{
	if (canBrew())
		brewPotion();
	else if (canCast())
		castSpell();
	else
		cout << "REST" << endl;
}

int main()
{
	while (true)
	{
		processInput();
		//printData();
		computeOutput();
	}
}