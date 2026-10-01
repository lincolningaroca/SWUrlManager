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
  ui->pteDesc->setPlaceholderText(tr("Description"));

}

QString MidleWidget::errorMessage(){

  const auto invalidUrlMsg = tr(
							   "<p>La dirección <b>\"%1\"</b> no es válida.</p>"
							   "<p>Una dirección URL válida debe tener una de las siguientes formas:"
							   "<ul>"
							   "<li>(http://www.)url.dominio</li>"
							   "<li>(https://www.)url.dominio</li>"
							   "<li>(ftp://)url.dominio</li>"
							   "<li>(ftp://www.)url.dominio</li>"
							   "</ul></p>"
							   "<p><b>Nota:</b> Los prefijos <i>http://</i>, <i>https://</i>, <i>ftp://</i> y <i>www.</i> son opcionales.<br>"
							   "Lo mínimo esperado es una dirección con el formato: <b>url.dominio</b></p>").arg(ui->txtUrl->text().trimmed());
  return invalidUrlMsg;

}



void MidleWidget::selectAndFocus() noexcept{

  ui->txtUrl->selectAll();
  ui->txtUrl->setFocus(Qt::OtherFocusReason);

}
