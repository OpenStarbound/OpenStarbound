#pragma once

#include <QErrorMessage>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QScrollBar>
#include <QTextEdit>

#include "json_tool.hpp"

namespace Star {

class JsonEditor : public QMainWindow {
  Q_OBJECT

public:
  explicit JsonEditor(JsonPath::PathPtr const& path, Options const& options, List<String> const& files);

private slots:
  void next();
  void back();
  void updatePreview(QString const& valueStr);

private:
  // Returns false if the change can't be made or the edit is invalid Json
  bool saveChanges();

  void displayCurrentFile();
  void updateValueEditor();
  void updateImagePreview();

  QLabel* m_statusLabel;
  QLabel* m_pathLabel;
  QLabel* m_imageLabel;
  QTextEdit* m_jsonPreview;
  QTextDocument* m_jsonDocument;
  QLineEdit* m_valueEditor;
  QErrorMessage* m_errorDialog;
  QPushButton* m_backButton;
  QPushButton* m_nextButton;

  JsonPath::PathPtr m_path;
  JsonInputFormatPtr m_editFormat;
  Options m_options;
  List<String> m_files;
  size_t m_fileIndex;
  FormattedJson m_currentJson;
};

int edit(int argc, char* argv[], JsonPath::PathPtr const& path, Options const& options, List<Input> const& inputs);

}
