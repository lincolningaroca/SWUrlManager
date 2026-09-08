#pragma once

#include <QWidget>

namespace Ui { class MidleWidget; }

class MidleWidget : public QWidget
{
  Q_OBJECT

public:
  explicit MidleWidget(QWidget *parent = nullptr);
  ~MidleWidget();

  QString url() const;
  QString description() const;

  void setInputsEnabled(bool enabled) noexcept;
  void selectAndFocus() noexcept;
  void clearInputs() noexcept;
  void clearInputsOnly() noexcept;

  void setUrl(const QString& url);
  void setDescription(const QString& desc);

  void applyIcons(const QColor& iconColor) noexcept;
  void restoreFont(const QString &family, int size, const QColor &color) noexcept;

  QString currentFont() const;
  QString textColor() const;
  int currentFontSize() const;

  void setPlacesHolders();

  QString errorMessage();

private:
  Ui::MidleWidget *ui;

signals:
  void urlTextChanged(const QString& text);
  void textColorChanged(const QColor& color);
};
