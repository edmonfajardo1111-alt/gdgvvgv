#include "raylib.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace {
constexpr int kCanvasWidth = 1280;
constexpr int kCanvasHeight = 800;
constexpr int kAnswerCount = 4;

const Color kInk{24, 38, 39, 255};
const Color kMuted{155, 177, 170, 255};
const Color kCream{246, 239, 216, 255};
const Color kCoral{222, 110, 79, 255};
const Color kGold{240, 190, 96, 255};
const Color kMint{112, 205, 164, 255};
const Color kPanel{28, 51, 50, 245};
const Color kLine{79, 112, 101, 255};
const Color kDoorFrame{87, 56, 43, 255};
const Color kDoorColor{173, 82, 58, 255};

struct Question {
    int level;
    const char* prompt;
    std::array<const char*, kAnswerCount> answers;
    int correctAnswer;
};

const std::array<Question, 9> kQuestions{{
    {1, "What is the main ingredient in classic hummus?",
     {"Chickpeas", "Lentils", "White beans", "Green peas"}, 0},
    {1, "Which herb gives classic pesto its signature flavor?",
     {"Mint", "Basil", "Parsley", "Dill"}, 1},
    {1, "What does 'al dente' mean when cooking pasta?",
     {"Very soft", "Cooked in cream", "Firm to the bite", "Served cold"}, 2},
    {2, "A roux is made by cooking which two ingredients together?",
     {"Cream and eggs", "Oil and water", "Sugar and butter", "Flour and fat"}, 3},
    {2, "What happens when you blanch vegetables?",
     {"Briefly boil, then cool quickly", "Roast until deeply browned", "Cook slowly in oil", "Dry them in the oven"}, 0},
    {2, "When you saute, how is the food cooked?",
     {"Submerged in water", "Quickly in a little hot fat", "Slowly under a lid", "Over indirect smoke"}, 1},
    {3, "In a professional kitchen, what is 'mise en place'?",
     {"A slow braise", "A pastry fold", "Everything prepared and in its place", "A finishing sauce"}, 2},
    {3, "Beurre blanc is an emulsion sauce built mainly from butter and what?",
     {"Tomatoes and stock", "Egg yolks and oil", "Cream and cheese", "White wine and vinegar"}, 3},
    {3, "A julienne cut produces what shape?",
     {"Thin matchsticks", "Small cubes", "Wide ribbons", "Round slices"}, 0}
}};

enum class Screen {
    Title,
    Doors,
    Quiz,
    Result
};

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float life;
    float size;
    Color color;
};

struct Game {
    Screen screen{Screen::Title};
    std::mt19937 random{std::random_device{}()};
    int winningDoor{0};
    int chosenDoor{-1};
    int questionIndex{0};
    int lastQuestionIndex{-1};
    std::array<int, kAnswerCount> answerOrder{0, 1, 2, 3};
    int correctChoice{0};
    int score{0};
    int streak{0};
    int bestScore{0};
    int bestStreak{0};
    int level{1};
    int bestLevel{1};
    int round{0};
    int roundPoints{0};
    bool won{false};
    bool foundWinningDoor{false};
    std::string resultMessage;
    float doorOpen{0.0f};
    float elapsed{0.0f};
    Font font{GetFontDefault()};
    bool ownsFont{false};
    std::vector<Particle> confetti;
};

const std::array<Rectangle, 3> kDoorRects{{
    {126.0f, 302.0f, 250.0f, 260.0f},
    {515.0f, 302.0f, 250.0f, 260.0f},
    {904.0f, 302.0f, 250.0f, 260.0f}
}};

void DrawCentered(const std::string& text, float y, float fontSize, Color color,
                  Font font, float spacing = 1.0f) {
    const Vector2 size = MeasureTextEx(font, text.c_str(), fontSize, spacing);
    DrawTextEx(font, text.c_str(), {kCanvasWidth * 0.5f - size.x * 0.5f, y},
               fontSize, spacing, color);
}

void DrawLeft(const std::string& text, float x, float y, float fontSize,
              Color color, Font font, float spacing = 1.0f) {
    DrawTextEx(font, text.c_str(), {x, y}, fontSize, spacing, color);
}

bool DrawButton(Rectangle bounds, const std::string& text, Vector2 mouse,
                Font font, bool primary = false, bool enabled = true) {
    const bool hovered = enabled && CheckCollisionPointRec(mouse, bounds);
    Color fill = primary ? kCoral : Color{44, 72, 66, 255};
    if (!enabled) fill = Color{52, 66, 61, 255};
    else if (hovered) fill = primary ? Color{238, 130, 93, 255} : Color{61, 94, 82, 255};

    DrawRectangleRounded(bounds, 0.16f, 12, fill);
    DrawRectangleLinesEx(bounds, hovered ? 2.0f : 1.0f,
                         hovered ? kGold : kLine);
    const Color textColor = enabled ? kCream : kMuted;
    const Vector2 size = MeasureTextEx(font, text.c_str(), 22.0f, 1.0f);
    DrawTextEx(font, text.c_str(),
               {bounds.x + (bounds.width - size.x) * 0.5f,
                bounds.y + (bounds.height - size.y) * 0.5f - 1.0f},
               22.0f, 1.0f, textColor);
    return hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
}

const char* RankName(int level) {
    switch (level) {
        case 2: return "LINE COOK";
        case 3: return "HEAD CHEF";
        default: return "HOME COOK";
    }
}

void LoadRecords(Game& game) {
    std::ifstream input("unlock_records.txt");
    if (input && input >> game.bestScore >> game.bestStreak) {
        input >> game.bestLevel;
        game.bestLevel = std::clamp(game.bestLevel, 1, 3);
        game.level = game.bestLevel;
    }
}

void SaveRecords(const Game& game) {
    std::ofstream output("unlock_records.txt", std::ios::trunc);
    if (output) output << game.bestScore << ' ' << game.bestStreak << ' ' << game.bestLevel << '\n';
}

void SpawnConfetti(Game& game) {
    std::uniform_real_distribution<float> xPosition(380.0f, 900.0f);
    std::uniform_real_distribution<float> xVelocity(-210.0f, 210.0f);
    std::uniform_real_distribution<float> yVelocity(-360.0f, -100.0f);
    std::uniform_real_distribution<float> size(4.0f, 9.0f);
    const std::array<Color, 4> colors{kGold, kMint, kCoral, kCream};

    game.confetti.clear();
    for (int i = 0; i < 85; ++i) {
        game.confetti.push_back({
            {xPosition(game.random), 390.0f},
            {xVelocity(game.random), yVelocity(game.random)},
            2.2f,
            size(game.random),
            colors[static_cast<std::size_t>(i) % colors.size()]
        });
    }
}

void StartRound(Game& game) {
    std::uniform_int_distribution<int> doorDistribution(0, 2);
    game.winningDoor = doorDistribution(game.random);
    game.chosenDoor = -1;
    game.round += 1;
    game.doorOpen = 0.0f;
    game.roundPoints = 0;
    game.won = false;
    game.foundWinningDoor = false;
    game.resultMessage.clear();
    game.confetti.clear();

    std::vector<int> eligibleQuestions;
    for (std::size_t i = 0; i < kQuestions.size(); ++i) {
        if (kQuestions[i].level == game.level && static_cast<int>(i) != game.lastQuestionIndex) {
            eligibleQuestions.push_back(static_cast<int>(i));
        }
    }
    if (eligibleQuestions.empty()) {
        for (std::size_t i = 0; i < kQuestions.size(); ++i) {
            if (kQuestions[i].level == game.level) eligibleQuestions.push_back(static_cast<int>(i));
        }
    }
    std::uniform_int_distribution<std::size_t> questionDistribution(0, eligibleQuestions.size() - 1);
    game.questionIndex = eligibleQuestions[questionDistribution(game.random)];
    game.lastQuestionIndex = game.questionIndex;

    game.answerOrder = {0, 1, 2, 3};
    std::shuffle(game.answerOrder.begin(), game.answerOrder.end(), game.random);
    const Question& question = kQuestions[static_cast<std::size_t>(game.questionIndex)];
    for (int i = 0; i < kAnswerCount; ++i) {
        if (game.answerOrder[static_cast<std::size_t>(i)] == question.correctAnswer) {
            game.correctChoice = i;
            break;
        }
    }
    game.screen = Screen::Doors;
}

void FinishRound(Game& game, bool won, const std::string& message) {
    game.won = won;
    game.resultMessage = message;
    game.screen = Screen::Result;

    if (won) {
        game.streak += 1;
        game.roundPoints = 100 + (game.streak - 1) * 25;
        game.score += game.roundPoints;
        if (game.streak % 3 == 0 && game.level < 3) {
            game.level += 1;
            game.resultMessage = "RANK UP! You are now a " + std::string(RankName(game.level)) +
                                 ". " + game.resultMessage;
        }
        game.bestLevel = std::max(game.bestLevel, game.level);
        if (game.score > game.bestScore || game.streak > game.bestStreak ||
            game.bestLevel < game.level) {
            game.bestScore = std::max(game.bestScore, game.score);
            game.bestStreak = std::max(game.bestStreak, game.streak);
            SaveRecords(game);
        }
        SpawnConfetti(game);
    } else {
        game.streak = 0;
    }
}

void DrawBackground(float elapsed) {
    DrawRectangleGradientV(0, 0, kCanvasWidth, kCanvasHeight,
                           Color{18, 42, 42, 255}, Color{12, 27, 31, 255});
    for (int i = 0; i < 16; ++i) {
        const float x = static_cast<float>((i * 97) % kCanvasWidth);
        const float wave = std::sin(elapsed * 0.35f + static_cast<float>(i)) * 10.0f;
        DrawCircleV({x, 92.0f + wave}, 2.0f, Fade(kMint, 0.22f));
        DrawLine(static_cast<int>(x), 120, static_cast<int>(x + 52.0f), 120,
                 Fade(kMint, 0.08f));
    }
    DrawRectangle(0, 0, kCanvasWidth, 5, kGold);
}

void DrawPill(float x, float y, float width, const std::string& label,
              const std::string& value, Font font) {
    const Rectangle bounds{x, y, width, 56.0f};
    DrawRectangleRounded(bounds, 0.28f, 12, Fade(kPanel, 0.95f));
    DrawRectangleLinesEx(bounds, 1.0f, kLine);
    DrawLeft(label, x + 16.0f, y + 8.0f, 12.0f, kMuted, font, 1.0f);
    DrawLeft(value, x + 16.0f, y + 25.0f, 21.0f, kCream, font, 1.0f);
}

void DrawDoor(const Game& game, int index, bool revealed, Vector2 mouse) {
    const Rectangle bounds = kDoorRects[static_cast<std::size_t>(index)];
    const bool hovered = game.screen == Screen::Doors && CheckCollisionPointRec(mouse, bounds);
    const bool isWinner = revealed && index == game.winningDoor;
    const bool isChosen = index == game.chosenDoor;

    DrawRectangleRounded({bounds.x - 10.0f, bounds.y - 10.0f,
                          bounds.width + 20.0f, bounds.height + 20.0f},
                         0.08f, 12, isWinner ? kGold : kDoorFrame);
    DrawRectangleRounded(bounds, 0.08f, 12, Color{12, 25, 26, 255});
    DrawRectangleRounded({bounds.x + 12.0f, bounds.y + 12.0f,
                          bounds.width - 24.0f, bounds.height - 24.0f},
                         0.07f, 12, Color{20, 37, 37, 255});

    float openAmount = 0.0f;
    if (isWinner && game.screen == Screen::Quiz) openAmount = game.doorOpen;
    if (isWinner && game.screen == Screen::Result && game.won) openAmount = 1.0f;

    const float inset = 17.0f;
    const float slide = (bounds.width - inset * 2.0f) * 0.72f * openAmount;
    const Rectangle door{bounds.x + inset + slide, bounds.y + inset,
                         bounds.width - inset * 2.0f - slide,
                         bounds.height - inset * 2.0f};
    Color doorColor = isWinner ? Color{55, 126, 96, 255} : kDoorColor;
    if (isChosen && !game.foundWinningDoor && game.screen == Screen::Result) {
        doorColor = Color{106, 74, 62, 255};
    }
    if (hovered) doorColor = Color{194, 96, 68, 255};

    DrawRectangleRec(door, doorColor);
    DrawRectangleLinesEx(door, hovered ? 3.0f : 2.0f, isWinner ? kGold : Color{222, 157, 111, 255});

    if (door.width > 60.0f) {
        const Rectangle upperPanel{door.x + 17.0f, door.y + 19.0f,
                                   door.width - 34.0f, door.height * 0.42f};
        const Rectangle lowerPanel{door.x + 17.0f, door.y + door.height * 0.51f,
                                   door.width - 34.0f, door.height * 0.32f};
        DrawRectangleLinesEx(upperPanel, 2.0f, Fade(kCream, 0.25f));
        DrawRectangleLinesEx(lowerPanel, 2.0f, Fade(kCream, 0.25f));
        DrawCircle(static_cast<int>(door.x + door.width - 22.0f),
                   static_cast<int>(door.y + door.height * 0.55f), 6.0f, kGold);
    }

    if (isWinner && revealed && openAmount > 0.2f) {
        DrawCircle(static_cast<int>(bounds.x + bounds.width * 0.5f),
                   static_cast<int>(bounds.y + 58.0f), 12.0f,
                   Fade(kGold, 0.15f + openAmount * 0.35f));
    }

    const std::string number = "0" + std::to_string(index + 1);
    const Vector2 numberSize = MeasureTextEx(game.font, number.c_str(), 18.0f, 2.0f);
    DrawTextEx(game.font, number.c_str(),
               {bounds.x + (bounds.width - numberSize.x) * 0.5f,
                bounds.y + bounds.height + 17.0f},
               18.0f, 2.0f, isChosen ? kGold : kMuted);
}

void DrawDoors(const Game& game, Vector2 mouse) {
    const bool revealed = game.screen == Screen::Quiz || game.screen == Screen::Result;
    for (int i = 0; i < 3; ++i) DrawDoor(game, i, revealed, mouse);
}

void DrawHud(const Game& game) {
    DrawPill(72.0f, 30.0f, 170.0f, "SCORE", std::to_string(game.score), game.font);
    DrawPill(257.0f, 30.0f, 185.0f, "KITCHEN STREAK", std::to_string(game.streak), game.font);
    DrawPill(472.0f, 30.0f, 240.0f, "CULINARY RANK", RankName(game.level), game.font);
    DrawPill(968.0f, 30.0f, 240.0f, "PERSONAL BEST", std::to_string(game.bestScore), game.font);
    DrawCentered("ROUND " + std::to_string(game.round), 111.0f, 13.0f, kGold, game.font, 2.0f);
}

void DrawRankProgress(const Game& game, float y) {
    const bool maxRank = game.level >= 3;
    const std::string label = maxRank
        ? "TOP RANK REACHED"
        : std::to_string(3 - game.streak % 3) + " WINS TO " + RankName(game.level + 1);
    DrawCentered(label, y, 12.0f, kMuted, game.font, 1.5f);

    const Rectangle track{490.0f, y + 21.0f, 300.0f, 7.0f};
    DrawRectangleRounded(track, 0.8f, 8, Fade(kLine, 0.8f));
    const float progress = maxRank ? 1.0f : static_cast<float>(game.streak % 3) / 3.0f;
    if (progress > 0.0f) {
        DrawRectangleRounded({track.x, track.y, track.width * progress, track.height},
                             0.8f, 8, kMint);
    }
}

void UpdateConfetti(Game& game, float deltaTime) {
    for (Particle& particle : game.confetti) {
        particle.position.x += particle.velocity.x * deltaTime;
        particle.position.y += particle.velocity.y * deltaTime;
        particle.velocity.y += 390.0f * deltaTime;
        particle.life -= deltaTime;
    }
    game.confetti.erase(
        std::remove_if(game.confetti.begin(), game.confetti.end(),
                       [](const Particle& particle) { return particle.life <= 0.0f; }),
        game.confetti.end());
}

void DrawConfetti(const Game& game) {
    for (const Particle& particle : game.confetti) {
        DrawRectanglePro({particle.position.x, particle.position.y,
                          particle.size, particle.size * 1.5f},
                         {particle.size * 0.5f, particle.size * 0.5f},
                         particle.position.x * 0.2f, Fade(particle.color, particle.life / 2.2f));
    }
}

void ChooseDoor(Game& game, int door) {
    game.chosenDoor = door;
    if (door == game.winningDoor) {
        game.foundWinningDoor = true;
        game.screen = Screen::Quiz;
    } else {
        FinishRound(game, false, "Wrong door. The winner was door " +
                    std::to_string(game.winningDoor + 1) + ".");
    }
}

void HandleDoorInput(Game& game, Vector2 mouse) {
    if (IsKeyPressed(KEY_ONE)) ChooseDoor(game, 0);
    else if (IsKeyPressed(KEY_TWO)) ChooseDoor(game, 1);
    else if (IsKeyPressed(KEY_THREE)) ChooseDoor(game, 2);

    if (game.screen != Screen::Doors || !IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) return;
    for (int i = 0; i < 3; ++i) {
        if (CheckCollisionPointRec(mouse, kDoorRects[static_cast<std::size_t>(i)])) {
            ChooseDoor(game, i);
            return;
        }
    }
}

void DrawTitle(Game& game, Vector2 mouse) {
    DrawCentered("THE CULINARY CHALLENGE", 145.0f, 17.0f, kGold, game.font, 4.0f);
    DrawCentered("Unlock the Door", 192.0f, 66.0f, kCream, game.font, 1.0f);
    DrawCentered("Three doors. One chance. A kitchen question stands between you and the win.",
                 282.0f, 19.0f, kMuted, game.font, 1.0f);

    DrawRectangleRounded({270.0f, 354.0f, 740.0f, 118.0f}, 0.08f, 12, Fade(kPanel, 0.9f));
    DrawRectangleLinesEx({270.0f, 354.0f, 740.0f, 118.0f}, 1.0f, kLine);
    DrawCentered("Build a streak: each correct answer earns points, with a bonus for every",
                 378.0f, 18.0f, kCream, game.font);
    DrawCentered("consecutive win. Your personal best is saved on this computer.",
                 409.0f, 18.0f, kMint, game.font);

    if (DrawButton({472.0f, 526.0f, 336.0f, 64.0f}, "START PLAYING", mouse, game.font, true)) {
        StartRound(game);
    }
    DrawCentered("Mouse or keys 1-3 to choose a door  |  Keys 1-4 to answer", 631.0f,
                 15.0f, kMuted, game.font);
    DrawCentered("Press ESC at any time to leave the round", 659.0f, 13.0f,
                 Fade(kMuted, 0.75f), game.font);
}

void DrawDoorScreen(Game& game, Vector2 mouse) {
    DrawHud(game);
    DrawCentered("CHOOSE YOUR DOOR", 154.0f, 24.0f, kCream, game.font, 3.0f);
    DrawCentered("Pick one. The culinary question only appears if you find the winner.",
                 198.0f, 17.0f, kMuted, game.font);
    DrawRankProgress(game, 237.0f);
    DrawDoors(game, mouse);
    DrawCentered("Click a door or press 1, 2, or 3", 638.0f, 17.0f, kGold, game.font);
    DrawCentered("A wrong door ends the round. Your streak resets, but your best score stays.",
                 672.0f, 14.0f, kMuted, game.font);
}

void AnswerQuestion(Game& game, int choice) {
    const bool correct = choice == game.correctChoice;
    const Question& question = kQuestions[static_cast<std::size_t>(game.questionIndex)];
    const std::string message = correct
        ? "Correct! The door unlocks."
        : "Not quite. The answer was " +
          std::string(question.answers[static_cast<std::size_t>(game.answerOrder[static_cast<std::size_t>(game.correctChoice)])]) + ".";
    FinishRound(game, correct, message);
}

void DrawQuiz(Game& game, Vector2 mouse) {
    DrawHud(game);
    DrawRankProgress(game, 172.0f);
    DrawDoors(game, mouse);

    DrawRectangleRounded({128.0f, 594.0f, 1024.0f, 176.0f}, 0.06f, 12, kPanel);
    DrawRectangleLinesEx({128.0f, 594.0f, 1024.0f, 176.0f}, 1.0f, kLine);
    DrawLeft(std::string(RankName(game.level)) + " CHALLENGE", 158.0f, 608.0f,
             13.0f, kGold, game.font, 2.0f);
    DrawLeft(kQuestions[static_cast<std::size_t>(game.questionIndex)].prompt,
             158.0f, 632.0f, 22.0f, kCream, game.font);

    constexpr float buttonWidth = 476.0f;
    constexpr float buttonHeight = 42.0f;
    for (int i = 0; i < kAnswerCount; ++i) {
        const int column = i % 2;
        const int row = i / 2;
        const Rectangle bounds{158.0f + column * 494.0f,
                               672.0f + row * 50.0f,
                               buttonWidth, buttonHeight};
        const int answerIndex = game.answerOrder[static_cast<std::size_t>(i)];
        const std::string label = std::to_string(i + 1) + "   " +
            kQuestions[static_cast<std::size_t>(game.questionIndex)]
                .answers[static_cast<std::size_t>(answerIndex)];
        if (DrawButton(bounds, label, mouse, game.font, false)) AnswerQuestion(game, i);
    }
}

void DrawResult(Game& game, Vector2 mouse) {
    DrawHud(game);
    DrawRankProgress(game, 172.0f);
    DrawDoors(game, mouse);

    DrawRectangleRounded({290.0f, 575.0f, 700.0f, 195.0f}, 0.07f, 12, kPanel);
    DrawRectangleLinesEx({290.0f, 575.0f, 700.0f, 195.0f}, 1.0f,
                         game.won ? Fade(kMint, 0.7f) : kLine);
    DrawCentered(game.won ? "DOOR UNLOCKED!" : "THE DOOR STAYS LOCKED", 596.0f,
                 25.0f, game.won ? kMint : kCoral, game.font, 2.0f);

        DrawCentered(game.resultMessage, 637.0f, 17.0f, kCream, game.font);
    DrawCentered("TOTAL " + std::to_string(game.score) + "     BEST " +
                 std::to_string(game.bestScore), 665.0f, 14.0f, kMuted, game.font);

    if (DrawButton({358.0f, 704.0f, 264.0f, 48.0f}, "PLAY AGAIN", mouse, game.font, true)) {
        StartRound(game);
    }
    if (DrawButton({658.0f, 704.0f, 264.0f, 48.0f}, "MAIN MENU", mouse, game.font)) {
        game.screen = Screen::Title;
    }
}

void HandleQuizKeys(Game& game) {
    if (IsKeyPressed(KEY_ONE)) AnswerQuestion(game, 0);
    else if (IsKeyPressed(KEY_TWO)) AnswerQuestion(game, 1);
    else if (IsKeyPressed(KEY_THREE)) AnswerQuestion(game, 2);
    else if (IsKeyPressed(KEY_FOUR)) AnswerQuestion(game, 3);
}
}

int main() {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(kCanvasWidth, kCanvasHeight, "Unlock the Door - Culinary Challenge");
    SetWindowMinSize(800, 600);
        const bool rankedUp = game.streak % 3 == 0 && game.level < 3;
        if (rankedUp) {

    Game game;
                                 ". +" + std::to_string(game.roundPoints) + " points.";
        } else {
            game.resultMessage = "Correct! +" + std::to_string(game.roundPoints) +
                                 " points. Streak: " + std::to_string(game.streak) + ".";
#if defined(_WIN32)
        const bool newBestLevel = game.level > game.bestLevel;
    if (FileExists("C:/Windows/Fonts/trebuc.ttf")) {
        Font font = LoadFontEx("C:/Windows/Fonts/trebuc.ttf", 64, nullptr, 0);
            newBestLevel) {
            game.font = font;
            game.ownsFont = true;
        }
    }
#endif

        game.resultMessage += " Your " + std::string(RankName(game.level)) + " rank is saved.";
    while (!WindowShouldClose()) {
        const float deltaTime = GetFrameTime();
        game.elapsed += deltaTime;
        UpdateConfetti(game, deltaTime);
        if (game.screen == Screen::Quiz) game.doorOpen = std::min(1.0f, game.doorOpen + deltaTime * 1.15f);

        const float scale = std::min(GetScreenWidth() / static_cast<float>(kCanvasWidth),
                                     GetScreenHeight() / static_cast<float>(kCanvasHeight));
        const Camera2D camera{{0.0f, 0.0f},
                              {(GetScreenWidth() - kCanvasWidth * scale) * 0.5f,
                               (GetScreenHeight() - kCanvasHeight * scale) * 0.5f},
                              0.0f, scale};
        const Vector2 mouse = GetScreenToWorld2D(GetMousePosition(), camera);

        if (IsKeyPressed(KEY_ESCAPE) && game.screen != Screen::Title) {
            game.screen = Screen::Title;
        }
        if (game.screen == Screen::Doors) HandleDoorInput(game, mouse);
        if (game.screen == Screen::Quiz) HandleQuizKeys(game);

        BeginDrawing();
        ClearBackground(Color{8, 20, 23, 255});
        BeginMode2D(camera);
        DrawBackground(game.elapsed);

        switch (game.screen) {
            case Screen::Title:
                DrawTitle(game, mouse);
                break;
            case Screen::Doors:
                DrawDoorScreen(game, mouse);
                break;
            case Screen::Quiz:
                DrawQuiz(game, mouse);
                break;
            case Screen::Result:
                DrawConfetti(game);
                DrawResult(game, mouse);
                break;
        }

        DrawLeft("UNLOCK THE DOOR  /  CULINARY TRIALS", 42.0f, 766.0f,
                 11.0f, Fade(kMuted, 0.65f), game.font, 2.0f);
        EndMode2D();
        EndDrawing();
    }

    if (game.ownsFont) UnloadFont(game.font);
    CloseWindow();
    return 0;
}
