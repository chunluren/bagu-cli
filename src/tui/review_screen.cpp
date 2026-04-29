#include "tui/review_screen.h"

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <sstream>

namespace bagu::tui {

using namespace ftxui;

namespace {

/// 把多行字符串切成 lines
std::vector<std::string> split_lines(const std::string& s) {
    std::vector<std::string> out;
    std::stringstream ss(s);
    std::string line;
    while (std::getline(ss, line)) out.push_back(line);
    if (out.empty()) out.push_back("");
    return out;
}

/// 把多行内容渲染为 vbox 的 paragraph elements
Element render_text_block(const std::string& body) {
    auto lines = split_lines(body);
    Elements rows;
    rows.reserve(lines.size());
    for (const auto& line : lines) {
        if (line.empty()) {
            rows.push_back(ftxui::text(""));
        } else {
            rows.push_back(paragraph(line));
        }
    }
    return vbox(std::move(rows));
}

}  // namespace

ReviewSessionStats run_review_tui(service::ReviewService& svc,
                                 std::vector<db::DueCard> cards) {
    ReviewSessionStats stats;
    stats.total = static_cast<int>(cards.size());

    if (cards.empty()) return stats;

    // 状态
    size_t cur_idx = 0;
    bool answer_revealed = false;
    auto card_start_time = std::chrono::steady_clock::now();
    std::string last_message;     // 评分结果反馈
    bool quit = false;
    bool show_help = false;

    auto screen = ScreenInteractive::Fullscreen();

    auto next_card = [&]() {
        cur_idx++;
        answer_revealed = false;
        last_message.clear();
        card_start_time = std::chrono::steady_clock::now();
    };

    auto submit = [&](int score) {
        if (cur_idx >= cards.size()) return;
        const auto& card = cards[cur_idx];
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - card_start_time).count();

        auto r = svc.submit_review(card.card_id, score, static_cast<int>(elapsed));
        stats.reviewed++;
        if (score >= 3) stats.correct++;

        if (r.is_ok()) {
            std::ostringstream ss;
            ss << "✓ saved: score=" << score
               << " · next in " << r.value().interval_days << " day(s)"
               << " · ease=" << r.value().ease_factor;
            last_message = ss.str();
        } else {
            last_message = "✗ save failed: " + r.error().message;
        }
        next_card();
    };

    // ========= 渲染器 =========
    auto renderer = Renderer([&] {
        if (cur_idx >= cards.size() || quit) {
            return vbox({
                text(""),
                text("  ✓ 本轮复习完成！") | bold | color(Color::Green),
                text(""),
                text("  Press any key to exit."),
            });
        }

        const auto& card = cards[cur_idx];

        // 顶部：进度
        std::string progress_str = "Card " + std::to_string(cur_idx + 1)
            + "/" + std::to_string(cards.size())
            + " · " + card.topic_name;
        if (card.is_new) progress_str += " · NEW";

        auto header = hbox({
            text("bagu review") | bold,
            filler(),
            text(progress_str) | color(Color::GrayLight),
        });

        // 进度条
        float ratio = static_cast<float>(cur_idx) / static_cast<float>(cards.size());
        auto progress_bar = gauge(ratio) | color(Color::Cyan);

        // 题目
        auto question_block = vbox({
            text(""),
            text("Q: ") | bold | color(Color::Yellow),
            paragraph(card.question) | bold,
        });

        // 答案区
        Element answer_block = answer_revealed
            ? vbox({
                separator(),
                text("A: ") | bold | color(Color::Green),
                render_text_block(card.answer) | yflex,
            })
            : vbox({
                separator(),
                text("  按 SPACE / Enter 显示答案") | dim | center,
            });

        // 底部提示
        Elements footer_items;
        if (answer_revealed) {
            footer_items = {
                text("[1]") | color(Color::Red), text(" 完全忘记 "),
                text("[2]") | color(Color::Red), text(" 答错 "),
                text("[3]") | color(Color::Yellow), text(" 答对(难) "),
                text("[4]") | color(Color::Green), text(" 答对(略犹豫) "),
                text("[5]") | color(Color::Green), text(" 完美 "),
                separator(),
                text("[s]") | color(Color::Blue), text(" 跳过 "),
                text("[q]") | color(Color::Blue), text(" 退出 "),
                text("[?]") | color(Color::Blue), text(" 帮助 "),
            };
        } else {
            footer_items = {
                text("[SPACE]") | color(Color::Cyan), text(" 显示答案 "),
                text("[s]") | color(Color::Blue), text(" 跳过 "),
                text("[q]") | color(Color::Blue), text(" 退出 "),
            };
        }
        auto footer = hbox(std::move(footer_items));

        Elements main_items = {
            header,
            text(""),
            progress_bar,
            text(""),
            question_block,
            answer_block | flex,
        };
        if (!last_message.empty()) {
            main_items.push_back(text(""));
            main_items.push_back(text(last_message) | dim | color(Color::Green));
        }
        if (show_help) {
            main_items.push_back(separator());
            main_items.push_back(text("帮助: SM-2 评分") | bold);
            main_items.push_back(paragraph("0-2 为答错，将重置间隔为 1 天；3-5 为答对，间隔按 ease factor 增长。"));
            main_items.push_back(text("再按 ? 关闭帮助"));
        }

        auto body = vbox(std::move(main_items)) | border;

        return vbox({
            body | flex,
            text(""),
            footer | center,
        });
    });

    // ========= 事件处理 =========
    auto component = CatchEvent(renderer, [&](Event event) {
        if (cur_idx >= cards.size()) {
            quit = true;
            screen.ExitLoopClosure()();
            return true;
        }

        if (event == Event::Character('q') || event == Event::Escape) {
            quit = true;
            stats.quit_early = static_cast<int>(cards.size() - cur_idx);
            screen.ExitLoopClosure()();
            return true;
        }
        if (event == Event::Character('?')) {
            show_help = !show_help;
            return true;
        }
        if (event == Event::Character('s')) {
            // 跳过：不评分，仅前进
            next_card();
            if (cur_idx >= cards.size()) {
                screen.ExitLoopClosure()();
            }
            return true;
        }

        if (!answer_revealed) {
            if (event == Event::Character(' ') || event == Event::Return) {
                answer_revealed = true;
                return true;
            }
        } else {
            // 评分
            for (char c = '0'; c <= '5'; ++c) {
                if (event == Event::Character(c)) {
                    submit(c - '0');
                    if (cur_idx >= cards.size()) {
                        screen.ExitLoopClosure()();
                    }
                    return true;
                }
            }
        }
        return false;
    });

    screen.Loop(component);
    return stats;
}

}  // namespace bagu::tui
