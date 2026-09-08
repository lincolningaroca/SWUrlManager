#pragma once

#include <QDialog>

class QTextEdit;
class QLineEdit;
class QComboBox;
class QPushButton;

class ReportBugDialog : public QDialog
{
  Q_OBJECT
public:
  explicit ReportBugDialog(QWidget *parent = nullptr);
  ~ReportBugDialog() override;

private slots:
  void on_reportOnGitHub();

private:
  void setupUi();
  void setupConnections();
  void collectSystemInfo();
  QString buildInfoText() const;
  QString severityLabel(int idx) const;
  bool validateFields();

  // UI
  QLineEdit     *m_subjectEdit{nullptr};
  QComboBox     *m_severityCombo{nullptr};
  QTextEdit     *m_descriptionEdit{nullptr};
  QPushButton   *m_btnReport{nullptr};
  QTextEdit     *m_systemInfoEdit{nullptr};

  // Datos
  QString m_systemInfo;
  QString m_appVersion;
};