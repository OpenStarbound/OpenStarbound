#pragma once

#include <QPlainTextEdit>

namespace Star {

class SPlainTextEdit : public QPlainTextEdit {
  Q_OBJECT
public:
  SPlainTextEdit(QWidget* parent = nullptr);

signals:
  void editingFinished();

protected:
  void focusOutEvent(QFocusEvent* e);

private slots:
  void wasChanged();

private:
  bool m_changed;
};

}
