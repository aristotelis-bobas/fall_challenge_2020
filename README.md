# fall_challenge_2020
My bot's source code for CodinGame's [Fall Challenge 2020](https://www.codingame.com/contests/fall-challenge-2020).

<a href="https://ibb.co/Wpckr89"><img src="https://i.ibb.co/YTtc5n9/Screenshot-from-2020-11-14-15-27-09.png" alt="Screenshot-from-2020-11-14-15-27-09" border="0"></a>

# day 1 - version 1
<p>
My first version's strategy is based on an abstract weight distribution system for the different available ingredients.
The weight is based on the amount of spells it takes to get a single unit of each ingredient.
By learning new spells the weighted value of an ingredient gets recalculated.
Using this weight system I calculated the efficiency of spells and recipes, and chose the most efficient spell / recipe each turn.
The spell to cast was found by recursively looking for the spell that would gain the most needed ingredients. </p>

Key flaws; <br>

- The weight system was too abstract; the exact amount of steps (including resting in between spells) were not taken into account
- Greedy algorithm: no complete simulation of all steps involved in a recipe, finds locally best recipe / spell
- Circular dependencies if I wanted to implement stone value dependencies in the weight distribution system

# day 3 - version 2

<p>
I reached the limit of the first strategy and started working on a new strategy to optimize the decision making process. I needed more accuracy in terms of amount of turns needed for each action, whether it was buying a spell from the tomebook or brewing a potion. I wrote simulations with different internal rules. These simulations enabled me to get an accurate overview of the amounts of step needed for each action. I used these simulations to find the most efficient recipe in terms of reward divided by amount of steps, and also used these simulations to rewrite the weight distribution system to actually get a more accurate weighted value of the creation rate of each of the 4 different ingredients. </p>

# day 5 - version 3

<p>
The simulations in the previous version were pre-defined by my own set rules. In this version I left the rules out of the simulations and implemented a depth-first searching algorithm that recursively digs deeper into the spread of possibilities. </p>
