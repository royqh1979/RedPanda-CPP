#include "imagemetainfomodel.h"
#include <TinyEXIF.h>
#include <cfloat>
#include <fstream>

ImageMetaInfoModel::ImageMetaInfoModel(QObject *parent):QStandardItemModel(parent)
{
}

ImageMetaInfoModel::~ImageMetaInfoModel()
{
}

inline QString fromStdString(const std::string &s) {
    return QString::fromUtf8(QByteArray::fromStdString(s));
}
void ImageMetaInfoModel::setImagePath(const QString& imagePath)
{
    if (imagePath!=mImagePath) {
        beginResetModel();
        mImagePath = imagePath;
        int parseResult{TinyEXIF::ErrorCode::PARSE_ABSENT_DATA};
        TinyEXIF::EXIFInfo imageEXIF;
        std::ifstream stream(imagePath.toLocal8Bit(), std::ios::binary);
        if (stream) {
            parseResult = imageEXIF.parseFrom(stream);
        }
        if (parseResult == TinyEXIF::PARSE_SUCCESS) {
            setColumnCount(2);
            QStandardItem *itemGPSInfos=new QStandardItem(tr("GPS Information"));
            if (imageEXIF.GeoLocation.hasLatLon()) {
                QString latitude = QString("%1°%2'%3\" %4").arg(int(imageEXIF.GeoLocation.LatComponents.degrees))
                        .arg(int(imageEXIF.GeoLocation.LatComponents.minutes))
                        .arg(int(imageEXIF.GeoLocation.LatComponents.seconds))
                        .arg(QChar(imageEXIF.GeoLocation.LatComponents.direction));   // Image latitude expressed as decimal
                itemGPSInfos->appendRow({
                                            new QStandardItem(tr("Latitude")),
                                            new QStandardItem(latitude)
                                        });
            }
            if (imageEXIF.GeoLocation.hasLatLon()) {
                QString longitude = QString("%1°%2'%3\" %4").arg(int(imageEXIF.GeoLocation.LonComponents.degrees))
                        .arg(int(imageEXIF.GeoLocation.LonComponents.minutes))
                        .arg(int(imageEXIF.GeoLocation.LonComponents.seconds))
                        .arg(QChar(imageEXIF.GeoLocation.LonComponents.direction));   // Image longitude expressed as decimal
                itemGPSInfos->appendRow({
                                            new QStandardItem(tr("Longitude")),
                                            new QStandardItem(longitude)
                                        });
            }
            if (imageEXIF.GeoLocation.hasAltitude())
                itemGPSInfos->appendRow({
                                            new QStandardItem(tr("Altitude")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.GeoLocation.Altitude))
                                        });
            if (itemGPSInfos->rowCount()>0)
                appendRow(itemGPSInfos);
            else
                delete itemGPSInfos;
            QStandardItem *itemCameraInfos=new QStandardItem(tr("Camera Information"));
            if (!imageEXIF.SerialNumber.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Serial Number")),
                                            new QStandardItem(fromStdString(imageEXIF.SerialNumber))
                                        });
            if (!imageEXIF.Model.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Model")),
                                            new QStandardItem(fromStdString(imageEXIF.Model))
                                        });
            if (!imageEXIF.Make.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Manufactor")),
                                            new QStandardItem(fromStdString(imageEXIF.Make))
                                        });
            if (!imageEXIF.CameraFirmware.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Camera Firmware")),
                                            new QStandardItem(fromStdString(imageEXIF.CameraFirmware))
                                        });
            if (!imageEXIF.Software.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Software")),
                                            new QStandardItem(fromStdString(imageEXIF.Software))
                                        });
            if (!imageEXIF.LensInfo.SerialNumber.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Lens Serial Number")),
                                            new QStandardItem(fromStdString(imageEXIF.LensInfo.SerialNumber))
                                        });
            if (!imageEXIF.LensInfo.Model.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Lens Model")),
                                            new QStandardItem(fromStdString(imageEXIF.LensInfo.Model))
                                        });
            if (!imageEXIF.LensInfo.Make.empty())
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Lens Manufactor")),
                                            new QStandardItem(fromStdString(imageEXIF.LensInfo.Make))
                                        });
            if (imageEXIF.LensInfo.FocalLengthMin!=0 && imageEXIF.LensInfo.FocalLengthMax!=0)
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Min/Max Focal Length")),
                                            new QStandardItem(QString("%1 / %2")
                                               .arg(imageEXIF.LensInfo.FocalLengthMin)
                                               .arg(imageEXIF.LensInfo.FocalLengthMax))
                                        });
            if (imageEXIF.LensInfo.FStopMin!=0 && imageEXIF.LensInfo.FStopMax!=0)
                itemCameraInfos->appendRow({
                                            new QStandardItem(tr("Min/Max FStop")),
                                            new QStandardItem(QString("%1 / %2")
                                               .arg(imageEXIF.LensInfo.FStopMin)
                                               .arg(imageEXIF.LensInfo.FStopMax))
                                        });
            if (itemCameraInfos->rowCount()>0)
                appendRow(itemCameraInfos);
            else
                delete itemCameraInfos;
    //http://en.wikipedia.org/wiki/Exif#/media/File:DigiKam_EXIF_information_screenshot.png
            QStandardItem *itemImageInfos=new QStandardItem(tr("Image Information"));
            if (!imageEXIF.Artist.empty())
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Artist")),
                                            new QStandardItem(fromStdString(imageEXIF.Artist))
                                        });
            if (!imageEXIF.Copyright.empty())
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Copyright")),
                                            new QStandardItem(fromStdString(imageEXIF.Copyright))
                                        });
            if (!imageEXIF.DateTime.empty())
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Date Time")),
                                            new QStandardItem(fromStdString(imageEXIF.DateTime))
                                        });
            if (!imageEXIF.ImageTitle.empty())
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Image Title")),
                                            new QStandardItem(fromStdString(imageEXIF.ImageTitle))
                                        });
            if (!imageEXIF.ImageDescription.empty())
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Image Description")),
                                            new QStandardItem(fromStdString(imageEXIF.ImageDescription))
                                        });
            if (imageEXIF.Rating!=0)
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Rating")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.Rating))
                                        });
            if (imageEXIF.RatingPercent!=0)
                itemImageInfos->appendRow({
                                            new QStandardItem(tr("Rating Percent")),
                                              new QStandardItem(tr("%1%").arg(imageEXIF.RatingPercent))
                                        });
            if (itemImageInfos->rowCount()>0)
                appendRow(itemImageInfos);
            else
                delete itemImageInfos;
            QStandardItem *itemPhotoInfos=new QStandardItem(tr("Photo Information"));
            if (imageEXIF.FocalLength!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Focal Length")),
                                            new QStandardItem(tr("%1 mm").arg(imageEXIF.FocalLength))
                                        });
            if (imageEXIF.LensInfo.FocalLengthIn35mm!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Focal length(35m film)")),
                                            new QStandardItem(tr("%1 mm").arg(imageEXIF.LensInfo.FocalLengthIn35mm))
                                        });
            if (imageEXIF.LensInfo.DigitalZoomRatio!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Digital Zoom Ratio")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.LensInfo.DigitalZoomRatio))
                                        });
            if (imageEXIF.ApertureValue != 0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Aperture")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.ApertureValue))
                                        });
            if (imageEXIF.FNumber!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("FNumber")),
                                            new QStandardItem(tr("f/%1").arg(imageEXIF.FNumber))
                                        });
            if (imageEXIF.ISOSpeedRatings!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("ISO Speed Rating")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.ISOSpeedRatings))
                                        });
            if (imageEXIF.ShutterSpeedValue!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Shutter Speed Value")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.ShutterSpeedValue))
                                        });

            if (imageEXIF.ExposureTime!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Exposure Time")),
                                            new QStandardItem(tr("1/%1 Seconds").arg(int(1/imageEXIF.ExposureTime)))
                                        });
            if (imageEXIF.BrightnessValue!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Brightness Value")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.BrightnessValue))
                                        });
            if (imageEXIF.ExposureBiasValue!=0)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Exposure Bias")),
                                            new QStandardItem(tr("%1").arg(imageEXIF.ExposureBiasValue))
                                        });
            if (imageEXIF.MeteringMode!=0) {
                QHash<int,QString> meteringModes{
                    {1,tr("Average")},
                    {2,tr("Center Weighted Average")},
                    {3,tr("Spot")},
                    {4,tr("Multi Spot")},
                    {5,tr("Pattern")},
                    {6,tr("Partial")},
                };
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Metering Mode")),
                                            new QStandardItem(meteringModes.value(imageEXIF.MeteringMode, tr("Unknown")))
                                        });
            }
            if (imageEXIF.ExposureProgram!=0) {
                QHash<int,QString> exposurePrograms{
                    {1,tr("Manual")},
                    {2,tr("Normal program")},
                    {3,tr("Aperture priority")},
                    {4,tr("Shutter priority")},
                    {5,tr("Creative program")},
                    {6,tr("Action program")},
                    {7,tr("Portrait mode")},
                    {8,tr("Landscape mode")},
                };
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Exposure Program")),
                                            new QStandardItem(exposurePrograms.value(imageEXIF.ExposureProgram,tr("Unknown")))
                                        });
            }
            if (imageEXIF.ExposureMode!=0xFFFF) {
                QHash<int,QString> exposureModes{
                    {0,tr("Auto")},
                    {1,tr("Manual")},
                    {2,tr("Auto bracket")},
                };
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Exposure Mode")),
                                            new QStandardItem(exposureModes.value(imageEXIF.ExposureMode,tr("Unknown")))
                                        });
            }
            if (imageEXIF.Flash)
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("Flash")),
                                            new QStandardItem((imageEXIF.Flash&1)?tr("On"):tr("Off"))
                                        });
            if (imageEXIF.WhiteBalance!=0xFFFF) {
                QHash<int,QString> whiteBalances{
                    {0,tr("Auto")},
                    {1,tr("Manual")},
                };
                itemPhotoInfos->appendRow({
                                            new QStandardItem(tr("White Balance")),
                                            new QStandardItem(whiteBalances.value(imageEXIF.WhiteBalance,tr("Unknown")))
                                        });
            }
            if (itemPhotoInfos->rowCount()>0)
                appendRow(itemPhotoInfos);
            else
                delete itemPhotoInfos;
        }
        if (rowCount()==0) {
            setColumnCount(1);
            appendRow({
                          new QStandardItem(tr("No Information"))
                      });
        }
        endResetModel();
    }
}
