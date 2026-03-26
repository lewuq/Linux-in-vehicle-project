#include "Weather/weather.h"
#include "ui_weather.h"

Weather::Weather(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Weather)
{
    ui->setupUi(this);
    this->resize(1024,600);

    // 顶部信息条避免文本被截断：固定标题宽度并给时间/地点更多可用空间。
    ui->label->setMaximumWidth(110);
    ui->label_37->setMaximumWidth(110);
    ui->label_Time->setMinimumWidth(210);
    ui->label_location->setMinimumWidth(120);
    ui->label_Time->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->label_location->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->horizontalLayout_2->setStretch(1, 4);
    ui->horizontalLayout_2->setStretch(2, 1);
    ui->horizontalLayout_2->setStretch(4, 2);
    ui->horizontalLayout_2->setStretch(5, 0);

    // 清理 weather.ui 中的内联样式，避免覆盖外部 QSS 主题。
    const QList<QWidget *> widgets = this->findChildren<QWidget *>();
    for (QWidget *w : widgets) {
        w->setStyleSheet(QString());
    }
    
    // ===== 加载浅色车机风QSS样式表（资源内置） =====
    const QString qssPath = ":/weather/Weather/weather_light_style.qss";
    QFile styleFile(qssPath);
    if (styleFile.open(QFile::ReadOnly)) {
        QString style = QLatin1String(styleFile.readAll());
        this->setStyleSheet(style);
        qDebug() << "[Weather] Loaded QSS:" << qssPath;
        styleFile.close();
    } else {
        qDebug() << "[Weather] QSS load failed, fallback inline style:" << qssPath;
        // 备用：如果QSS文件不存在，使用内联QSS
        QString globalStyleSheet = R"(
            QMainWindow { background-color: #F3F5F8; }
            QGroupBox { background-color: #FAFBFC; border-radius: 20px; border: 1px solid #E6E8EC; padding: 15px; }
            QLabel { background-color: transparent; color: #111827; font-family: "Microsoft YaHei", Roboto, "思源黑体"; }
            QLabel#label_now_tmp { font-size: 72px; font-weight: bold; color: #0F172A; }
        )";
        this->setStyleSheet(globalStyleSheet);
    }
    
    request.setUrl(QUrl("http://v1.yiketianqi.com/free/week?appid=84215575&appsecret=HAb0SMQ9&unescape=1&city=惠州"));
    netManager.get(request);
    connect(&netManager,SIGNAL(finished(QNetworkReply *)),this,SLOT(getWeatherInfo(QNetworkReply *)));
}

Weather::~Weather()
{
    delete ui;
}

/* 将日期转换为星期  */
QString Weather::TransDataToWeek(QString Data)
{
    QDate date;
    date =  QDate::fromString(Data,"yyyy-MM-dd");
    int week = date.dayOfWeek();
    switch (week) {
    case 1:
        return "星期一";
    case 2:
        return "星期二";
    case 3:
        return "星期三";
    case 4:
        return "星期四";
    case 5:
        return "星期五";
    case 6:
        return "星期六";
    case 7:
        return "星期七";
    default:
        return "星期零";
    }
}

/* 将 xx-xx 转换为xx月xx日  */
QString Weather::TransDataToMyData(QString Data)
{
    QString MM = Data.split("-").at(1);
    QString DD = Data.split("-").at(2);
    return QString("%1月%2日").arg(MM).arg(DD);
}

/* 拼接显示 昼夜温度  */
QString Weather::TransTempToStr(QString tmpDay, QString tmpNight)
{
    return QString("%1~%2 ℃").arg(tmpDay).arg(tmpNight);
}

/* 根据天气选择对应图片  */
QString Weather::SelectWeatherImg(QString WeatherDes)
{
    return QString(":/weather/Weather/images/%1.png").arg(WeatherDes);
}

void Weather::updateInfo()
{
    netManager.get(request);
}


/* 根据网络请求结果解析得到天气数据  并转换为合适格式后显示  */
void Weather::getWeatherInfo(QNetworkReply *reply)
{
    QByteArray bytes = reply->readAll();
    int num = bytes.size();
        //qDebug()<<bytes;
    QJsonDocument doc = QJsonDocument::fromJson(bytes);
    if(!doc.isObject()){
        qDebug()<<"Netjson not an jsonObject!";
        ui->label_now_des->setText("网络异常");
        reply->deleteLater();
        return;
    }
    if(num<60)
    {
        ui->label_now_des->setText("服务异常");
        reply->deleteLater();
        return;
    }

    weather_data.clear();
    weather_wea.clear();
    weather_wea_img.clear();
    weather_tem_day.clear();
    weather_tem_night.clear();
    weather_win.clear();
    weather_win_speed.clear();

    QJsonObject obj = doc.object();
   // QString cityId = obj.value("cityid").toString();
    cityName = obj.value("city").toString();
    UpdateTime = obj.value("update_time").toString();
    QJsonArray arrays = obj.value("data").toArray();
    for(int i=0;i<7;i++)
    {
        weather_data.append(arrays[i].toObject().value("date").toString());
        weather_wea.append(arrays[i].toObject().value("wea").toString());
        weather_tem_day.append(arrays[i].toObject().value("tem_day").toString());
        weather_tem_night.append(arrays[i].toObject().value("tem_night").toString());
        weather_win.append(arrays[i].toObject().value("win").toString());
        weather_win_speed.append(arrays[i].toObject().value("win_speed").toString());
        weather_wea_img.append(arrays[i].toObject().value("wea_img").toString());
    }

    ui->label_day1_data->setText(TransDataToMyData(weather_data.at(1)));
    ui->label_day2_data->setText(TransDataToMyData(weather_data.at(2)));
    ui->label_day3_data->setText(TransDataToMyData(weather_data.at(3)));
    ui->label_day4_data->setText(TransDataToMyData(weather_data.at(4)));

    ui->label_day1_week->setText(TransDataToWeek(weather_data.at(1)));
    ui->label_day2_week->setText(TransDataToWeek(weather_data.at(2)));
    ui->label_day3_week->setText(TransDataToWeek(weather_data.at(3)));
    ui->label_day4_week->setText(TransDataToWeek(weather_data.at(4)));

    ui->label_day1_des->setText(weather_wea.at(1));
    ui->label_day2_des->setText(weather_wea.at(2));
    ui->label_day3_des->setText(weather_wea.at(3));
    ui->label_day4_des->setText(weather_wea.at(4));

    ui->label_day1_win->setText(weather_win.at(1));
    ui->label_day2_win->setText(weather_win.at(2));
    ui->label_day3_win->setText(weather_win.at(3));
    ui->label_day4_win->setText(weather_win.at(4));

    ui->label_day1_winSpeed->setText(weather_win_speed.at(1));
    ui->label_day2_winSpeed->setText(weather_win_speed.at(2));
    ui->label_day3_winSpeed->setText(weather_win_speed.at(3));
    ui->label_day4_winSpeed->setText(weather_win_speed.at(4));

    ui->label_day1_tmp->setText(TransTempToStr(weather_tem_day.at(1),weather_tem_night.at(1)));
    ui->label_day2_tmp->setText(TransTempToStr(weather_tem_day.at(2),weather_tem_night.at(2)));
    ui->label_day3_tmp->setText(TransTempToStr(weather_tem_day.at(3),weather_tem_night.at(3)));
    ui->label_day4_tmp->setText(TransTempToStr(weather_tem_day.at(4),weather_tem_night.at(4)));

    QPixmap pixmap;

    // ===== 修复天气图标锯齿：关闭粗糙拉伸，开启平滑抗锯齿 =====
    // 未来四天天气图标
    ui->label_day1_img->setScaledContents(false);
    ui->label_day2_img->setScaledContents(false);
    ui->label_day3_img->setScaledContents(false);
    ui->label_day4_img->setScaledContents(false);
    
    pixmap.load(SelectWeatherImg(weather_wea_img.at(1)));
    pixmap = pixmap.scaled(ui->label_day1_img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_day1_img->setPixmap(pixmap);
    
    pixmap.load(SelectWeatherImg(weather_wea_img.at(2)));
    pixmap = pixmap.scaled(ui->label_day2_img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_day2_img->setPixmap(pixmap);
    
    pixmap.load(SelectWeatherImg(weather_wea_img.at(3)));
    pixmap = pixmap.scaled(ui->label_day3_img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_day3_img->setPixmap(pixmap);
    
    pixmap.load(SelectWeatherImg(weather_wea_img.at(4)));
    pixmap = pixmap.scaled(ui->label_day4_img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_day4_img->setPixmap(pixmap);

    // 当前天气大图标
    ui->label_now_img->setScaledContents(false);
    pixmap.load(SelectWeatherImg(weather_wea_img.at(0)));
    pixmap = pixmap.scaled(ui->label_now_img->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->label_now_img->setPixmap(pixmap);
    ui->label_now_des->setText(weather_wea.at(0));
    ui->label_now_win->setText(weather_win.at(0));
    ui->label_now_winSpeed->setText(weather_win_speed.at(0));
    ui->label_now_tmp->setText(weather_tem_day.at(0));

    ui->label_Time->setText(UpdateTime);
    ui->label_location->setText(cityName);
    
    // ===== 工程约束：网络请求数据及时释放 =====
    // 【关键】JSON解析完成后删除reply对象，防止内存堆积
    // 特别是对于长期运行的车载系统，及时释放很重要
    reply->deleteLater();

}


void Weather::on_pushButton_clicked()
{
    updateInfo();
}

void Weather::on_pushButton_2_clicked()
{
    this->hide();
}
