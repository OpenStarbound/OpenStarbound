#include "StarSPlainTextEdit.hpp"

namespace Star {

SPlainTextEdit::SPlainTextEdit(QWidget* parent)
  : QPlainTextEdit(parent), m_changed(false) {

  connect(this, SIGNAL(textChanged()), this, SLOT(wasChanged()));
}

void SPlainTextEdit::focusOutEvent(QFocusEvent* e) {
  QPlainTextEdit::focusOutEvent(e);
  m_changed = false;
  emit editingFinished();
}

void SPlainTextEdit::wasChanged() {
  m_changed = true;
}

}
