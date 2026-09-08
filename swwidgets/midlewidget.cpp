#include "midlewidget.hpp"
#include "ui_midlewidget.h"

MidleWidget::MidleWidget(QWidget *parent)
  : QWidget(parent), ui(new Ui::MidleWidget)
{
  ui->setupUi(this);

  connect(ui->txtUrl, &QLineEdit::textChanged, this, &MidleWidget::urlTextChanged);
  connect(ui->pteDesc, &SWTextEdit::textColorChanged, this, &MidleWidget::textColorChanged);
}

MidleWidget::~MidleWidget()
{
  delete ui;
}

QString MidleWidget::url() const{ return ui->txtUrl->text();}

QString MidleWidget::description() const{ return ui->pteDesc->toHtml();}

void MidleWidget::setInputsEnabled(bool enabled) noexcept{

  ui->txtUrl->setEnabled(enabled);
  ui->pteDesc->setEnabled(enabled);

}

void MidleWidget::clearInputs() noexcept {
  clearInputsOnly();
  ui->txtUrl->setFocus(Qt::OtherFocusReason);
}
void MidleWidget::clearInputsOnly() noexcept {
  ui->txtUrl->clear();
  ui->pteDesc->clear();

}

void MidleWidget::setUrl(const QString &url){

  ui->txtUrl->setText(url);

}

void MidleWidget::setDescription(const QString &desc){

  ui->pteDesc->setHtml(desc);

}

void MidleWidget::applyIcons(const QColor &iconColor) noexcept{

  ui->pteDesc->applyIcons(iconColor);

}

void MidleWidget::restoreFont(const QString &family, int size, const QColor &color) noexcept{

  ui->pteDesc->restoreFont(family, size, color);

}

QString MidleWidget::currentFont() const
{
  return ui->pteDesc->currentFont();
}

QString MidleWidget::textColor() const
{
  return ui->pteDesc->editor()->textColor().name(QColor::HexRgb);

}

int MidleWidget::currentFontSize() const
{
  return ui->pteDesc->currentFontSize();
}



void MidleWidget::setPlacesHolders(){

  ui->txtUrl->setPlaceholderText(QStringLiteral("(http:// | https:// | ftp://)(www.)url.com(.pe | .abc)"));
  ui->pteDesc->setPlaceholderText(QStringLiteral("Description to url's"));

}

QString MidleWidget::errorMessage(){

  const auto invalidUrlMsg = QString("<p>"
									 "<span>"
									 "La dirección: <strong>\"%1\"</strong>, no es válida!<br>"
									 "una dirección url válida debe tener una de las siguiente formas:"
									 "<ol>"
									 "<li><strong>(http://www.)url.dominio</strong></li>"
									 "<li><strong>(https://www.)url.dominio</strong></li>"
									 "<li><strong>(ftp://)url.dominio</strong></li>"
									 "<li><strong>(ftp://www.)url.dominio</strong></li>"
									 "</ol>"
									 "<br>Nota:<br>"
									 "Tenga en cuenta que "
									 "<strong>http://, https://, ftp://, www.</strong> son opcionales<br>"
									 "Lo mínimo que se espera es una direccón de la forma: <strong>\"url.domino\"</strong>"
									 "</span>"
									 "</p>").arg(ui->txtUrl->text());
  return invalidUrlMsg;

}



void MidleWidget::selectAndFocus() noexcept{

  ui->txtUrl->selectAll();
  ui->txtUrl->setFocus(Qt::OtherFocusReason);

}
