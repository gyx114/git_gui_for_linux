#include <gtkmm.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <cstdio>
#include <memory>
#include <array>

//这是kh的测试
// Git 命令执行器
class GitExecutor
{
public:
    static std::string shell_quote(const std::string &input)
    {
        std::string out = "'";
        for (char c : input)
        {
            if (c == '\'')
                out += "'\\''";
            else
                out += c;
        }
        out += "'";
        return out;
    }

    static std::string exec(const std::string &cmd)
    {
        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
        if (!pipe)
            return "Error: Failed to run command";
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
        {
            result += buffer.data();
        }
        return result;
    }

    static std::string trim_trailing_newlines(const std::string &text)
    {
        std::string out = text;
        while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        {
            out.pop_back();
        }
        return out;
    }

    static bool is_git_repo(const std::string &repo_path)
    {
        std::string output = exec("git -C " + shell_quote(repo_path) + " rev-parse --is-inside-work-tree 2>/dev/null");
        return output.find("true") != std::string::npos;
    }

    static std::string get_repo_root(const std::string &repo_path)
    {
        return trim_trailing_newlines(
            exec("git -C " + shell_quote(repo_path) + " rev-parse --show-toplevel 2>/dev/null"));
    }

    static std::string get_status(const std::string &repo_path)
    {
        return exec("git -C " + shell_quote(repo_path) + " status --porcelain");
    }

    static void add_all(const std::string &repo_path)
    {
        exec("git -C " + shell_quote(repo_path) + " add .");
    }

    static void stage(const std::string &repo_path, const std::string &file)
    {
        exec("git -C " + shell_quote(repo_path) + " add " + shell_quote(file));
    }

    static void unstage(const std::string &repo_path, const std::string &file)
    {
        exec("git -C " + shell_quote(repo_path) + " reset HEAD " + shell_quote(file));
    }

    static void commit(const std::string &repo_path, const std::string &message)
    {
        exec("git -C " + shell_quote(repo_path) + " commit -m " + shell_quote(message));
    }

    static std::string get_diff(const std::string &repo_path, const std::string &file)
    {
        return exec("git -C " + shell_quote(repo_path) + " diff " + shell_quote(file));
    }

    static std::string get_log(const std::string &repo_path, int count = 10)
    {
        return exec("git -C " + shell_quote(repo_path) + " log --oneline -" + std::to_string(count));
    }

    static std::string push(const std::string &repo_path)
    {
        return exec("git -C " + shell_quote(repo_path) + " push 2>&1");
    }
};

// 主窗口类
class GitGUI : public Gtk::Window
{
public:
    GitGUI()
    {
        set_title("Git GUI");
        set_default_size(900, 600);

        // 创建主布局
        auto main_box = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_VERTICAL, 5);
        main_box->set_margin_top(10);
        main_box->set_margin_bottom(10);
        main_box->set_margin_start(10);
        main_box->set_margin_end(10);

        // === 顶部工具栏 ===
        auto toolbar = Gtk::make_managed<Gtk::Box>(Gtk::ORIENTATION_HORIZONTAL, 5);

        m_select_repo_btn = Gtk::make_managed<Gtk::Button>("选择仓库");
        m_add_all_btn = Gtk::make_managed<Gtk::Button>("Add All");
        m_refresh_btn = Gtk::make_managed<Gtk::Button>("刷新");
        m_commit_btn = Gtk::make_managed<Gtk::Button>("提交");
        m_push_btn = Gtk::make_managed<Gtk::Button>("Push");
        m_commit_btn->set_sensitive(false);

        m_select_repo_btn->signal_clicked().connect(sigc::mem_fun(*this, &GitGUI::on_select_repo));
        m_add_all_btn->signal_clicked().connect(sigc::mem_fun(*this, &GitGUI::on_add_all));
        m_refresh_btn->signal_clicked().connect(sigc::mem_fun(*this, &GitGUI::on_refresh));
        m_commit_btn->signal_clicked().connect(sigc::mem_fun(*this, &GitGUI::on_commit));
        m_push_btn->signal_clicked().connect(sigc::mem_fun(*this, &GitGUI::on_push));

        toolbar->pack_start(*m_select_repo_btn, Gtk::PACK_SHRINK);
        toolbar->pack_start(*m_add_all_btn, Gtk::PACK_SHRINK);
        toolbar->pack_start(*m_refresh_btn, Gtk::PACK_SHRINK);
        toolbar->pack_start(*m_commit_btn, Gtk::PACK_SHRINK);
        toolbar->pack_start(*m_push_btn, Gtk::PACK_SHRINK);

        // 提交消息输入框
        m_commit_entry = Gtk::make_managed<Gtk::Entry>();
        m_commit_entry->set_placeholder_text("提交消息...");
        m_commit_entry->signal_changed().connect([this]()
                                                 { m_commit_btn->set_sensitive(!m_commit_entry->get_text().empty()); });

        toolbar->pack_start(*m_commit_entry, Gtk::PACK_EXPAND_WIDGET);

        // === 中间区域：文件列表和 Diff ===
        auto hpaned = Gtk::make_managed<Gtk::Paned>(Gtk::ORIENTATION_HORIZONTAL);

        // 左侧：文件列表（TreeView）
        m_file_list = Gtk::make_managed<Gtk::TreeView>();
        create_file_list_model();

        auto scrolled_left = Gtk::make_managed<Gtk::ScrolledWindow>();
        scrolled_left->add(*m_file_list);
        scrolled_left->set_hexpand(true);
        scrolled_left->set_vexpand(true);
        hpaned->pack1(*scrolled_left, true, 300);

        // 右侧：Diff 显示
        m_diff_view = Gtk::make_managed<Gtk::TextView>();
        m_diff_view->set_editable(false);
        m_diff_view->set_wrap_mode(Gtk::WRAP_WORD);

        auto diff_scrolled = Gtk::make_managed<Gtk::ScrolledWindow>();
        diff_scrolled->add(*m_diff_view);
        diff_scrolled->set_hexpand(true);
        diff_scrolled->set_vexpand(true);
        hpaned->pack2(*diff_scrolled, true, true);

        // === 底部：状态栏 ===
        m_status_bar = Gtk::make_managed<Gtk::Label>();
        m_status_bar->set_halign(Gtk::ALIGN_START);
        m_status_bar->set_margin_top(5);
        m_status_bar->set_margin_bottom(5);
        m_status_bar->set_margin_start(5);
        m_status_bar->set_margin_end(5);

        // 组装
        main_box->pack_start(*toolbar, Gtk::PACK_SHRINK);
        main_box->pack_start(*hpaned, Gtk::PACK_EXPAND_WIDGET);
        main_box->pack_start(*m_status_bar, Gtk::PACK_SHRINK);

        add(*main_box);
        show_all_children();

        m_repo_path = ".";
        if (GitExecutor::is_git_repo(m_repo_path))
        {
            m_repo_path = GitExecutor::get_repo_root(m_repo_path);
            on_refresh();
        }
        else
        {
            m_repo_path.clear();
            m_status_bar->set_text("请先点击“选择仓库”选择一个 Git 仓库目录");
        }
    }

private:
    void on_select_repo()
    {
        Gtk::FileChooserDialog dialog(*this, "选择 Git 仓库目录", Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
        dialog.add_button("取消", Gtk::RESPONSE_CANCEL);
        dialog.add_button("选择", Gtk::RESPONSE_OK);

        if (dialog.run() == Gtk::RESPONSE_OK)
        {
            const std::string selected = dialog.get_filename();
            if (GitExecutor::is_git_repo(selected))
            {
                m_repo_path = GitExecutor::get_repo_root(selected);
                on_refresh();
                m_status_bar->set_text("已选择仓库: " + m_repo_path);
            }
            else
            {
                Gtk::MessageDialog error_dialog(
                    *this,
                    "所选目录不是 Git 仓库",
                    false,
                    Gtk::MESSAGE_ERROR,
                    Gtk::BUTTONS_OK,
                    true);
                error_dialog.set_secondary_text("请重新选择包含 .git 的目录。");
                error_dialog.run();
            }
        }
    }

    void create_file_list_model()
    {
        m_file_model = Gtk::ListStore::create(m_columns);
        m_file_list->set_model(m_file_model);

        // 添加列
        m_file_list->append_column("状态", m_columns.status);
        m_file_list->append_column("文件名", m_columns.filename);

        // 选择文件时显示 diff
        m_file_list->get_selection()->signal_changed().connect(
            sigc::mem_fun(*this, &GitGUI::on_file_selected));
    }

    void on_refresh()
    {
        if (m_repo_path.empty())
        {
            m_status_bar->set_text("请先点击“选择仓库”选择一个 Git 仓库目录");
            m_file_model->clear();
            m_diff_view->get_buffer()->set_text("");
            return;
        }

        if (!GitExecutor::is_git_repo(m_repo_path))
        {
            m_status_bar->set_text("所选目录不是 Git 仓库，请重新选择");
            m_file_model->clear();
            m_diff_view->get_buffer()->set_text("");
            return;
        }

        m_status_bar->set_text("刷新中...");

        // 刷新文件列表
        m_file_model->clear();

        std::string status = GitExecutor::get_status(m_repo_path);
        std::istringstream iss(status);
        std::string line;

        while (std::getline(iss, line))
        {
            if (line.length() >= 3)
            {
                auto row = *m_file_model->append();
                row[m_columns.status] = line.substr(0, 2);
                row[m_columns.filename] = line.substr(3);
            }
        }

        m_status_bar->set_text("就绪 - " + std::to_string(m_file_model->children().size()) + " 个更改");
    }

    void on_file_selected()
    {
        auto selection = m_file_list->get_selection();
        auto iter = selection->get_selected();
        if (iter)
        {
            Glib::ustring filename_glib = (*iter)[m_columns.filename];
            std::string filename = filename_glib;
            std::string diff = GitExecutor::get_diff(m_repo_path, filename);

            auto buffer = m_diff_view->get_buffer();
            buffer->set_text(diff);
        }
    }

    void on_commit()
    {
        if (m_repo_path.empty())
        {
            m_status_bar->set_text("请先选择 Git 仓库");
            return;
        }

        std::string msg = m_commit_entry->get_text();
        if (msg.empty())
            return;

        GitExecutor::commit(m_repo_path, msg);
        m_commit_entry->set_text("");
        on_refresh();

        // 清空 diff
        m_diff_view->get_buffer()->set_text("");

        m_status_bar->set_text("提交成功！");
    }

    void on_add_all()
    {
        if (m_repo_path.empty())
        {
            m_status_bar->set_text("请先选择 Git 仓库");
            return;
        }

        GitExecutor::add_all(m_repo_path);
        on_refresh();
        m_status_bar->set_text("已执行 git add .");
    }

    void on_push()
    {
        if (m_repo_path.empty())
        {
            m_status_bar->set_text("请先选择 Git 仓库");
            return;
        }

        std::string output = GitExecutor::push(m_repo_path);
        std::string result = GitExecutor::trim_trailing_newlines(output);
        if (result.empty())
        {
            result = "Push 已执行，未返回输出。";
        }

        Gtk::MessageDialog dialog(
            *this,
            "Push 结果",
            false,
            Gtk::MESSAGE_INFO,
            Gtk::BUTTONS_OK,
            true);
        dialog.set_secondary_text(result);
        dialog.run();

        on_refresh();
    }

    // TreeView 列结构
    class ModelColumns : public Gtk::TreeModel::ColumnRecord
    {
    public:
        ModelColumns()
        {
            add(status);
            add(filename);
        }
        Gtk::TreeModelColumn<Glib::ustring> status;
        Gtk::TreeModelColumn<Glib::ustring> filename;
    };

    ModelColumns m_columns;
    Glib::RefPtr<Gtk::ListStore> m_file_model;

    Gtk::TreeView *m_file_list = nullptr;
    Gtk::TextView *m_diff_view = nullptr;
    Gtk::Entry *m_commit_entry = nullptr;
    Gtk::Button *m_select_repo_btn = nullptr;
    Gtk::Button *m_add_all_btn = nullptr;
    Gtk::Button *m_refresh_btn = nullptr;
    Gtk::Button *m_commit_btn = nullptr;
    Gtk::Button *m_push_btn = nullptr;
    Gtk::Label *m_status_bar = nullptr;
    std::string m_repo_path;
};

int main(int argc, char *argv[])
{
    auto app = Gtk::Application::create(argc, argv, "com.gitgui");

    // 检查是否在 Git 仓库目录运行
    std::cout << "Git GUI 启动" << std::endl;
    std::cout << "请确保在 Git 仓库目录中运行" << std::endl;

    GitGUI window;
    return app->run(window);
}
