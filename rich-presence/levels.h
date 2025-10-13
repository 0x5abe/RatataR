#pragma once
#include <Windows.h>
int*** playerObjects = (int***)(0x7C58FC);
int(__cdecl* getID)(const char* name, int startingIndx) = (int(__cdecl*)(const char*, int))0x58D670;
struct Level {
    char key;
    const char* name;
    const char* imageName;
};
Level levelList[] = {
    { 0, "In Menu", "bootingup" },
    { 1, "Somewhere in France", "somewhere_in_france" },
    { 2, "Destiny River", "destiny_river" },
    { 3, "Home Stink Home", "home_stink_home" },
    { 4, "Paris Streets", "paris_streets" },
    { 5, "The City of Lights", "the_city_of_lights" },
    { 6, "How'd You Make That?", "bootingup" },
    { 7, "Little Chef - Big Kitchen", "little_chef_big_kitchen" },
    { 8, "Chop Chop Chase", "bootingup" },
    { 9, "The City Market", "the_city_market" },
    { 10, "Harried Grocery Havoc", "bootingup" },
    { 11, "Cake & Bake", "bootingup" },
    { 12, "This Ain't no Cakewalk", "bootingup" },
    { 13, "Soup, Line, and Sinker", "bootingup" },
    { 14, "Ratatouille", "bootingup" },
    { 15, "Soupy Assistance", "soupy_assistance" },
    { 16, "Peeling Potatoes", "bootingup" },
    { 17, "Appetite for Appetizers", "bootingup" },
    { 18, "Salad Ballad", "bootingup" },
    { 19, "Stacking for a Salad", "bootingup" },
    { 20, "Take Aim and Blow", "bootingup" },
    { 21, "Sound the Alarm!", "bootingup" },
    { 22, "Shocking Starter", "bootingup" },
    { 23, "The Desserted Kitchen", "the_desserted_kitchen" },
    { 24, "Was the Food That Bad?", "bootingup" },
    { 25, "Fishing for treasures", "bootingup" },
    { 26, "Food Fishing", "bootingup" },
    { 27, "Lean, Mean, Dishwashing Machine", "bootingup" },
    { 28, "Spick & Span Pumpkins", "bootingup" },
    { 29, "Wait for the Crepe", "bootingup" },
    { 30, "Rat Race", "bootingup" },
    { 31, "Closet Collection", "bootingup" },
    { 32, "Last Rat Standing", "bootingup" },
    { 33, "Tightrope Chaos", "bootingup" },
    { 34, "That's a Loot of Fruit", "thats_a_loot_of_fruit" },
    { 35, "This is a Steakout!", "this_is_a_steakout" },
    { 36, "Say Cheese!", "say_cheese" },
    { 37, "Dirty Dish Fright", "dirty_dish_fright" },
    { 38, "Pasta Persuasion", "pasta_persuasion" },
    { 39, "Desserted Wonderland", "desserted_wonderland" },
    { 40, "Veggie Vault", "veggie_vault" },
    { 41, "Soaring Strawberries", "soaring_strawberries" },
    { 42, "Kitchen Chaos", "kitchen_chaos" },
    { 43, "Sausage Shenanigan", "sausage_shenanigan" },
    { 44, "Underground Fun", "underground_fun" },
    { 45, "Leaky Pipes", "leaky_pipes" },
    { 46, "The Slide of Your Life", "the_slide_of_your_life" },
    { 47, "Kitchen Pipe", "kitchen_pipe" },
    { 48, "Oh Smelly Water", "oh_smelly_water" },
    { 49, "Test_Mar", "bootingup" },
    { 50, "Test_Connex", "bootingup" },
    { 51, "Test_Nico", "bootingup" },
    { 52, "Test_Julien", "bootingup" }
};
Level getLevel()
{
    while (*((int*)0x007DE8A8) == 0) {

        Sleep(1000);
    }

    int levelID = *(int*)(*(int*)(*(int*)(*(int*)(0x007DE8A8) + 8) + 0x9C4) + 0x7C);
    if (levelID > 52 || levelID < 0)
    {
        return levelList[0];
    }
    return levelList[levelID];
}

const char* getCharName()
{
    while (*((int*)0x7C58FC) == 0) {

        Sleep(1000);
    }

    //int charID = *(*(*playerObjects) + 4);
    int charID = (*(int*)(*(int*)(*(int*)0x7C58FC) + 4));
    if (charID == getID("P_REMY", 0)) {
        return "Playing as Remy";
    }
    else if (charID == getID("P_L_REMY", 0)) {
        return "Playing as Linguini";
    }
    else if (charID == getID("P_EMILE", 0)) {
        return "Playing as Emile";
    }
    else if (charID == getID("P_REMY_C", 0)) {
        return "Playing as Remy (Chase)";
    }
    else if (charID == getID("P_REMY_R", 0)) {
        return "Playing as Remy (Raft)";
    }
    else if (charID == getID("P_LINGUI", 0)) {
        return "Playing as Linguini (Beta)";
    }
    else if (charID == getID("P_L_CAKE", 0)) {
        return "Playing as Linguini (Cake Minigame)";
    }
    else if (charID == getID("P_R_BKD", 0)) {
        return "Playing as Git (Rope Minigame)";
    }
    else if (charID == getID("P_REMY_S", 0)) {
        return "Playing as Remy (Sephamore)";
    }
    else if (charID == getID("P_REMY_T", 0)) {
        return "Playing as Serge (Heist Minigame)";
    }
    else if (charID == getID("P_R_SWMK", 0)){
        return "Playing as Celine (Conveyor Minigame)";
    }
    else if (charID == getID("P_FISH",0)) {
        return "Playing as Fishing Hook (Fishing Minigame) (How???)";
    }
    else if (charID == getID("P_R_WCCT", 0)) {
        return "Playing as Twitch (Wire Connecting Minigame)";
    }
    else if (charID == getID("P_R_SWKN", 0)) {
        return "Playing as Remy (Blender Minigame)";
    }
    else if (charID == getID("P_L_POTA", 0)) {
        return "Playing as Linguini (Potato Minigame)";
    }
    else if (charID == getID("P_L_WASH", 0)) {
        return "Playing as Linguini (Hose Minigame) (Also how???)";
    }
    else if (charID == getID("P_L_WASH2", 0)) {
        return "Playing as Linguini (Hose Minigame) (Also how???)";
    }
    else if (charID == getID("P_PLAT01", 0)) {
        return "Playing as Linguini (Salad Minigame)";
    }
    else if (charID == getID("P_SOUP01", 0)) {
        return "Playing as Linguini (Soup Minigame)";
    }
    else if (charID == getID("P_L_CREP", 0)) {
        return "Playing as Linguini (Crepe Minigame) (How???)";
    }
    else if (charID == getID("P_R_SOUP", 0)) {
        return "Playing as Remy (Soup Minigame)";
    }
    else if (charID == getID("P_R_COL", 0)) {
        return "Playing as Colette (Dinner Rush)";
    }
    return "Playing as Remy";
}