/*
 * Copyright 2026 jwall64
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <QObject>
#include <QtQml>
#include <QColor>

#include <Core/Types.hpp>
#include <Core/Optional.hpp>

namespace nbl::frontend {

class InstallationDetails : public QObject {

    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(qint64 installationId READ installationId CONSTANT)
    Q_PROPERTY(qint64 gameId READ gameId CONSTANT)
    Q_PROPERTY(QByteArray type READ type CONSTANT)
    Q_PROPERTY(QVariantMap fields READ fields CONSTANT)
    Q_PROPERTY(QByteArray title READ title CONSTANT)
    Q_PROPERTY(QByteArray description READ description CONSTANT)
    Q_PROPERTY(qint64 poster READ poster CONSTANT)
    Q_PROPERTY(qint64 banner READ banner CONSTANT)
    Q_PROPERTY(qint64 icon READ icon CONSTANT)
    Q_PROPERTY(qint64 logo READ logo CONSTANT)
    Q_PROPERTY(quint64 logoPosition READ logoPosition CONSTANT)
    Q_PROPERTY(quint64 playTime READ playTime CONSTANT)
    Q_PROPERTY(quint64 playCount READ playCount CONSTANT)
    Q_PROPERTY(quint64 lastPlayed READ lastPlayed CONSTANT)
    Q_PROPERTY(quint64 added READ added CONSTANT)
    Q_PROPERTY(quint64 releaseDate READ releaseDate CONSTANT)
    Q_PROPERTY(double rating READ rating CONSTANT)
    Q_PROPERTY(QVariant colorOverride READ colorOverride CONSTANT)
    Q_PROPERTY(QByteArray gameTitle READ gameTitle CONSTANT)
    Q_PROPERTY(QByteArray gameDescription READ gameDescription CONSTANT)
    Q_PROPERTY(qint64 gamePoster READ gamePoster CONSTANT)
    Q_PROPERTY(qint64 gameBanner READ gameBanner CONSTANT)
    Q_PROPERTY(qint64 gameIcon READ gameIcon CONSTANT)
    Q_PROPERTY(qint64 gameLogo READ gameLogo CONSTANT)
    Q_PROPERTY(quint64 gameLogoPosition READ gameLogoPosition CONSTANT)

    Q_PROPERTY(QByteArray resolvedTitle READ resolvedTitle CONSTANT)
    Q_PROPERTY(QUrl resolvedPosterUrl READ resolvedPosterUrl CONSTANT)
    Q_PROPERTY(QUrl resolvedBannerUrl READ resolvedBannerUrl CONSTANT)
    Q_PROPERTY(QUrl resolvedIconUrl READ resolvedIconUrl CONSTANT)
    Q_PROPERTY(QUrl resolvedLogoUrl READ resolvedLogoUrl CONSTANT)
    Q_PROPERTY(quint64 resolvedLogoPosition READ resolvedLogoPosition CONSTANT)

public:

    InstallationDetails(QObject* parent = nullptr) : 
        QObject(parent),
        _mInstallationId(0),
        _mGameId(0),
        _mType{},
        _mFields{},
        _mTitle{},
        _mDescription{},
        _mPoster{},
        _mBanner{},
        _mIcon{},
        _mLogo{},
        _mLogoPosition(0),
        _mPlayTime(0),
        _mPlayCount(0),
        _mLastPlayed(0),
        _mAdded(0),
        _mReleaseDate(0),
        _mRating(0.0),
        _mColorOverride{},
        _mGameTitle{},
        _mGameDescription{},
        _mGamePoster{},
        _mGameBanner{},
        _mGameIcon{},
        _mGameLogo{},
        _mGameLogoPosition(0)
    {}

    InstallationDetails(
        qint64 installationId,
        qint64 gameId,
        QByteArray type,
        QVariantMap fields,
        QByteArray title,
        QByteArray description,
        qint64 poster,
        qint64 banner,
        qint64 icon,
        qint64 logo,
        quint64 logoPosition,
        quint64 playTime,
        quint64 playCount,
        quint64 lastPlayed,
        quint64 added,
        quint64 releaseDate,
        f64 rating,
        Optional<QColor> colorOverride,
        QByteArray gameTitle,
        QByteArray gameDescription,
        qint64 gamePoster,
        qint64 gameBanner,
        qint64 gameIcon,
        qint64 gameLogo,
        quint64 gameLogoPosition,
        QObject* parent = nullptr
    ) : 
        QObject(parent),
        _mInstallationId(installationId),
        _mGameId(gameId),
        _mType(type),
        _mFields(fields),
        _mTitle(title),
        _mDescription(description),
        _mPoster(poster),
        _mBanner(banner),
        _mIcon(icon),
        _mLogo(logo),
        _mLogoPosition(logoPosition),
        _mPlayTime(playTime),
        _mPlayCount(playCount),
        _mLastPlayed(lastPlayed),
        _mAdded(added),
        _mReleaseDate(releaseDate),
        _mRating(rating),
        _mColorOverride(colorOverride),
        _mGameTitle(gameTitle),
        _mGameDescription(gameDescription),
        _mGamePoster(gamePoster),
        _mGameBanner(gameBanner),
        _mGameIcon(gameIcon),
        _mGameLogo(gameLogo),
        _mGameLogoPosition(gameLogoPosition)
    {}

    InstallationDetails(InstallationDetails&& other) noexcept :
        QObject(other.parent()),
        _mInstallationId(other._mInstallationId),
        _mGameId(other._mGameId),
        _mType(std::move(other._mType)),
        _mFields(std::move(other._mFields)),
        _mTitle(std::move(other._mTitle)),
        _mDescription(std::move(other._mDescription)),
        _mPoster(other._mPoster),
        _mBanner(other._mBanner),
        _mIcon(other._mIcon),
        _mLogo(other._mLogo),
        _mLogoPosition(other._mLogoPosition),
        _mPlayTime(other._mPlayTime),
        _mPlayCount(other._mPlayCount),
        _mLastPlayed(other._mLastPlayed),
        _mAdded(other._mAdded),
        _mReleaseDate(other._mReleaseDate),
        _mRating(other._mRating),
        _mColorOverride(other._mColorOverride),
        _mGameTitle(std::move(other._mGameTitle)),
        _mGameDescription(std::move(other._mGameDescription)),
        _mGamePoster(other._mGamePoster),
        _mGameBanner(other._mGameBanner),
        _mGameIcon(other._mGameIcon),
        _mGameLogo(other._mGameLogo),
        _mGameLogoPosition(std::move(other._mGameLogoPosition))
    { }

    InstallationDetails& operator=(InstallationDetails&& other) noexcept {
        if(this == &other) return *this;

        setParent(other.parent());
        _mInstallationId = other._mInstallationId;
        _mGameId = other._mGameId;
        _mType = other._mType;
        _mFields = other._mFields;
        _mTitle = other._mTitle;
        _mDescription = other._mDescription;
        _mPoster = other._mPoster;
        _mBanner = other._mBanner;
        _mIcon = other._mIcon;
        _mLogo = other._mLogo;
        _mLogoPosition = other._mLogoPosition;
        _mPlayTime = other._mPlayTime;
        _mPlayCount = other._mPlayCount;
        _mLastPlayed = other._mLastPlayed;
        _mAdded = other._mAdded;
        _mReleaseDate = other._mReleaseDate;
        _mRating = other._mRating;
        _mColorOverride = other._mColorOverride;
        _mGameTitle = other._mGameTitle;
        _mGameDescription = other._mGameDescription;
        _mGamePoster = other._mGamePoster;
        _mGameBanner = other._mGameBanner;
        _mGameIcon = other._mGameIcon;
        _mGameLogo = other._mGameLogo;
        _mGameLogoPosition = other._mGameLogoPosition;

        return *this;
    }

    qint64 installationId() const { return _mInstallationId; }
    qint64 gameId() const { return _mGameId; }
    QByteArray type() const { return _mType; }
    QVariantMap fields() const { return _mFields; }
    QByteArray title() const { return _mTitle; }
    QByteArray description() const { return _mDescription; }
    qint64 poster() const { return _mPoster; }
    qint64 banner() const { return _mBanner; }
    qint64 icon() const { return _mIcon; }
    qint64 logo() const { return _mLogo; }
    quint64 logoPosition() const { return _mLogoPosition; }
    quint64 playTime() const { return _mPlayTime; }
    quint64 playCount() const { return _mPlayCount; }
    quint64 lastPlayed() const { return _mLastPlayed; }
    quint64 added() const { return _mAdded; }
    quint64 releaseDate() const { return _mReleaseDate; }
    f64 rating() const { return _mRating; }
    QVariant colorOverride() const { return _mColorOverride.hasValue() ? *_mColorOverride : QVariant(); }

    QByteArray gameTitle() const { return _mGameTitle; }
    QByteArray gameDescription() const { return _mGameDescription; }
    qint64 gamePoster() const { return _mGamePoster; }
    qint64 gameBanner() const { return _mGameBanner; }
    qint64 gameIcon() const { return _mGameIcon; }
    qint64 gameLogo() const { return _mGameLogo; }
    quint64 gameLogoPosition() const { return _mGameLogoPosition; }

    QByteArray resolvedTitle() const {
        return _mTitle.length() == 0 ? _mGameTitle : _mTitle;
    }
    QUrl resolvedPosterUrl() const {
        return _mediaUrl(_mPoster == 0 ? _mGamePoster : _mPoster);
    }
    QUrl resolvedBannerUrl() const {
        return _mediaUrl(_mBanner == 0 ? _mGameBanner : _mBanner);
    }
    QUrl resolvedIconUrl() const {
        return _mediaUrl(_mIcon == 0 ? _mGameIcon : _mIcon);
    }
    QUrl resolvedLogoUrl() const {
        return _mediaUrl(_mLogo == 0 ? _mGameLogo : _mLogo);
    }
    quint64 resolvedLogoPosition() const {
        return _mLogoPosition == 4 ? _mGameLogoPosition : _mLogoPosition;
    }

private:

    QUrl _mediaUrl(qint64 refId) const;
    
    qint64 _mInstallationId;
    qint64 _mGameId;
    QByteArray _mType;
    QVariantMap _mFields;
    QByteArray _mTitle;
    QByteArray _mDescription;
    qint64 _mPoster;
    qint64 _mBanner;
    qint64 _mIcon;
    qint64 _mLogo;
    quint64 _mLogoPosition;
    quint64 _mPlayTime;
    quint64 _mPlayCount;
    quint64 _mLastPlayed;
    quint64 _mAdded;
    quint64 _mReleaseDate;
    f64 _mRating;
    nbl::Optional<QColor> _mColorOverride;
    QByteArray _mGameTitle;
    QByteArray _mGameDescription;
    qint64 _mGamePoster;
    qint64 _mGameBanner;
    qint64 _mGameIcon;
    qint64 _mGameLogo;
    quint64 _mGameLogoPosition;

};

}
