#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <QObject>
#include <QThreadPool>
#include <QRunnable>
#include <QFunctionPointer>

// Lightweight async dispatcher backed by QThreadPool.
// All tasks run on a shared pool limited to 1 worker thread so that
// long-running operations (jackd start, pw-cli load, etc.) never block
// the UI thread.
class TaskRunner : public QObject {
    Q_OBJECT

public:
    explicit TaskRunner(QObject* parent = nullptr);
    ~TaskRunner() override;

    // Submit a work function to run in a worker thread.
    // On completion, `done` is emitted on the main (GUI) thread.
    // If an exception/error occurs, `fail` is emitted instead.
    void submit(const QString& label,
                QFunctionPointer work,   // void* arg -> void  (arg ignored)
                QFunctionPointer done,   // void* arg -> void  (label passed as QString pointer)
                QFunctionPointer fail);  // void* arg -> void  (QString error)

    // Convenience: submit with lambdas (captures preserved via std::function).
    void run(const QString& label, std::function<void()> work,
             std::function<void()> done,
             std::function<void(const QString&)> fail = nullptr);

    // Minimal overload: just work + label, no callbacks.
    void run(const QString& label, std::function<void()> work) {
        run(label, std::move(work), []() {}, [](const QString&) {});
    }

signals:
    void taskDone(const QString& label);
    void taskFailed(const QString& label, const QString& error);

private:
    class Task : public QRunnable {
    public:
        Task(const QString& label,
             std::function<void()> work,
             std::function<void()> done,
             std::function<void(const QString&)> fail);
        void run() override;

    private:
        QString m_label;
        std::function<void()> m_work;
        std::function<void()> m_done;
        std::function<void(const QString&)> m_fail;
    };

    QThreadPool m_pool;
};

#endif // TASK_RUNNER_H
