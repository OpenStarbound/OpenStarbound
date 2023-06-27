#include <QApplication>
#include <QGridLayout>
#include <QPushButton>
#include "StarFile.hpp"
#include "json_tool.hpp"
#include "editor_gui.hpp"

using namespace Star;

int const ImagePreviewWidth = 300;
int const ImagePreviewHeight = 600;

JsonEditor::JsonEditor(JsonPath::PathPtr const& path, Options const& options, List<String> const& files)
  : QMainWindow(), m_path(path), m_editFormat(options.editFormat), m_options(options), m_files(files), m_fileIndex(0) {
  auto* w = new QWidget(this);
  w->setObjectName("background");
  setCentralWidget(w);
  setWindowTitle("Json Editor");
  resize(1280, 720);

  auto* layout = new QGridLayout(centralWidget());

  QFont font("Monospace");
  font.setStyleHint(QFont::StyleHint::Monospace);

  m_jsonDocument = new QTextDocument("Hello world");
  m_jsonDocument->setDefaultFont(font);

  m_statusLabel = new QLabel(centralWidget());
  layout->addWidget(m_statusLabel, 0, 0, 1, 5);

  m_jsonPreview = new QTextEdit(this);
  m_jsonPreview->setReadOnly(true);
  m_jsonPreview->setDocument(m_jsonDocument);
  layout->addWidget(m_jsonPreview, 1, 0, 1, 5);

  m_backButton = new QPushButton(centralWidget());
  m_backButton->setText("« Back");
  layout->addWidget(m_backButton, 2, 0);
  connect(m_backButton, SIGNAL(pressed()), this, SLOT(back()));

  m_pathLabel = new QLabel(centralWidget());
  layout->addWidget(m_pathLabel, 2, 1);

  m_imageLabel = new QLabel(centralWidget());
  m_imageLabel->setMaximumSize(ImagePreviewWidth, ImagePreviewHeight);
  m_imageLabel->setMinimumSize(ImagePreviewWidth, ImagePreviewHeight);
  m_imageLabel->setAlignment(Qt::AlignCenter);
  if (!m_options.editorImages.empty())
    layout->addWidget(m_imageLabel, 1, 5);

  m_valueEditor = new QLineEdit(centralWidget());
  m_valueEditor->setFont(font);
  layout->addWidget(m_valueEditor, 2, 2);
  connect(m_valueEditor, SIGNAL(returnPressed()), this, SLOT(next()));
  connect(m_valueEditor, SIGNAL(textChanged(QString const&)), this, SLOT(updatePreview(QString const&)));

  m_nextButton = new QPushButton(centralWidget());
  m_nextButton->setText("Next »");
  m_nextButton->setDefault(true);
  layout->addWidget(m_nextButton, 2, 3);
  connect(m_nextButton, SIGNAL(pressed()), this, SLOT(next()));

  m_errorDialog = new QErrorMessage(this);
  m_errorDialog->setModal(true);

  displayCurrentFile();
}

void JsonEditor::next() {
  if (!m_valueEditor->isEnabled() || saveChanges()) {
    ++m_fileIndex;
    if (m_fileIndex >= m_files.size()) {
      close();
      return;
    }

    displayCurrentFile();
  }
}

void JsonEditor::back() {
  if (m_fileIndex == 0)
    return;

  --m_fileIndex;
  displayCurrentFile();
}

void JsonEditor::updatePreview(QString const& valueStr) {
  try {
    FormattedJson newValue = m_editFormat->toJson(valueStr.toStdString());
    FormattedJson preview = addOrSet(false, m_path, m_currentJson, m_options.insertLocation, newValue);
    m_jsonDocument->setPlainText(preview.repr().utf8Ptr());

  } catch (JsonException const&) {
  } catch (JsonParsingException const&) {
    // Don't update the preview if it's not valid Json.
  }
}

bool JsonEditor::saveChanges() {
  try {
    FormattedJson newValue = m_editFormat->toJson(m_valueEditor->text().toStdString());
    m_currentJson = addOrSet(false, m_path, m_currentJson, m_options.insertLocation, newValue);
    String repr = reprWithLineEnding(m_currentJson);
    File::writeFile(repr, m_files.get(m_fileIndex));
    return true;

  } catch (StarException const& e) {
    m_errorDialog->showMessage(e.what());
    return false;
  }
}

void JsonEditor::displayCurrentFile() {
  String file = m_files.get(m_fileIndex);

  size_t progress = (m_fileIndex + 1) * 100 / m_files.size();
  String status = strf("Editing file {}/{} ({}%):    {}", m_fileIndex + 1, m_files.size(), progress, file);
  m_statusLabel->setText(status.utf8Ptr());

  m_backButton->setEnabled(m_fileIndex != 0);
  m_nextButton->setText(m_fileIndex == m_files.size() - 1 ? "Done" : "Next »");

  m_pathLabel->setText(m_path->path().utf8Ptr());

  m_imageLabel->setText("No preview");
  m_jsonDocument->setPlainText("");
  m_valueEditor->setText("");
  m_valueEditor->setEnabled(false);

  try {
    m_currentJson = FormattedJson::parse(File::readFileString(file));

    m_jsonDocument->setPlainText(m_currentJson.repr().utf8Ptr());

    updateValueEditor();

    updateImagePreview();

  } catch (StarException const& e) {
    // Something else went wrong (maybe while parsing the document) and allowing
    // the user to edit this file might cause us to lose data.
    m_errorDialog->showMessage(e.what());
  }

  m_jsonPreview->moveCursor(QTextCursor::Start);

  m_valueEditor->selectAll();
  m_valueEditor->setFocus(Qt::FocusReason::OtherFocusReason);
}

void JsonEditor::updateValueEditor() {
  FormattedJson value;
  try {
    value = m_path->get(m_currentJson);
  } catch (JsonPath::TraversalException const&) {
    // Path does not already exist in the Json document. We're adding it.
    value = m_editFormat->getDefault();
  }

  String valueText;
  try {
    valueText = m_editFormat->fromJson(value);
  } catch (JsonException const& e) {
    // The value already present in the was no thte type we expected, e.g.
    // it was an int, when we wanted a string array for CSV.
    // Clear the value already present.
    m_errorDialog->showMessage(e.what());
    valueText = m_editFormat->fromJson(m_editFormat->getDefault());
  }
  m_valueEditor->setText(valueText.utf8Ptr());
  m_valueEditor->setEnabled(true);
}

void JsonEditor::updateImagePreview() {
  String file = m_files.get(m_fileIndex);
  for (JsonPath::PathPtr const& imagePath : m_options.editorImages) {
    try {
      String image = imagePath->get(m_currentJson).toJson().toString();
      image = File::relativeTo(File::dirName(file), image.extract(":"));

      QPixmap pixmap = QPixmap(image.utf8Ptr()).scaledToWidth(ImagePreviewWidth);
      if (pixmap.height() > ImagePreviewHeight)
        pixmap = pixmap.scaledToHeight(ImagePreviewHeight);
      m_imageLabel->setPixmap(pixmap);
      break;
    } catch (JsonPath::TraversalException const&) {
    }
  }
}

int Star::edit(int argc, char* argv[], JsonPath::PathPtr const& path, Options const& options, List<Input> const& inputs) {
  QApplication app(argc, argv);
  StringList files;
  for (Input const& input : inputs) {
    if (input.is<FindInput>())
      files.appendAll(findFiles(input.get<FindInput>()));
    else
      files.append(input.get<FileInput>().filename);
  }
  JsonEditor e(path, options, files);
  e.show();
  return app.exec();
}
