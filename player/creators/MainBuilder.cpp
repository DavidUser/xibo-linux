#include "MainBuilder.hpp"

#include "creators/ImageFactory.hpp"
#include "creators/VideoFactory.hpp"
#include "creators/AudioFactory.hpp"
#include "creators/WebViewFactory.hpp"

#include "creators/BackgroundBuilder.hpp"
#include "creators/MainLayoutBuilder.hpp"
#include "creators/MediaContainerBuilder.hpp"

#include "media/GetMediaPosition.hpp"
#include "utils/Resources.hpp"

namespace LayoutXlf = ResourcesXlf::Layout;
namespace RegionXlf = ResourcesXlf::Region;
namespace MediaXlf = ResourcesXlf::Media;

std::unique_ptr<IMainLayout> MainBuilder::buildLayoutWithChildren(const xlf_node& tree)
{
    return buildLayout(tree.get_child(LayoutXlf::NodeName));
}

std::unique_ptr<IMainLayout> MainBuilder::buildLayout(const xlf_node& layoutNode)
{
    int width = LayoutXlf::width(layoutNode);
    int height = LayoutXlf::height(layoutNode);

    MainLayoutBuilder builder;
    return builder.width(width).height(height).background(buildBackground(layoutNode))
                  .mediaContainers(collectContainers(layoutNode)).build();
}

std::unique_ptr<IBackground> MainBuilder::buildBackground(const xlf_node& layoutNode)
{
    int width = LayoutXlf::width(layoutNode);
    int height = LayoutXlf::height(layoutNode);
    auto path = LayoutXlf::backgroundPath(layoutNode);
    auto color = LayoutXlf::backgroundColor(layoutNode);

    return BackgroundBuilder().width(width).height(height).path(path).color(color).build();
}

std::vector<MediaContainerWithPos> MainBuilder::collectContainers(const xlf_node& layoutNode)
{
    std::vector<MediaContainerWithPos> containers;
    for(auto [nodeName, containerNode] : layoutNode)
    {
        if(nodeName == RegionXlf::NodeName)
        {
            int x = RegionXlf::left(containerNode);
            int y = RegionXlf::top(containerNode);
            containers.emplace_back(MediaContainerWithPos{buildContainer(containerNode), x, y});
        }
    }
    return containers;
}

std::unique_ptr<IMediaContainer> MainBuilder::buildContainer(const xlf_node& containerNode)
{
    int width = RegionXlf::width(containerNode);
    int height = RegionXlf::height(containerNode);
    auto zorder = RegionXlf::zindex(containerNode);
    auto loop = RegionXlf::loop(containerNode);

    MediaContainerBuilder builder;
    return builder.width(width).height(height).zorder(zorder).loop(loop)
                  .visibleMedia(collectVisibleMedia(containerNode))
                  .invisibleMedia(collectInvisibleMedia(containerNode)).build();
}

std::vector<MediaWithPos> MainBuilder::collectVisibleMedia(const xlf_node& containerNode)
{
    std::vector<MediaWithPos> media;
    for(auto [nodeName, mediaNode] : containerNode)
    {
        if(nodeName == MediaXlf::NodeName && MediaXlf::type(mediaNode) != MediaXlf::AudioType) // FIXME condition
        {
            auto builtMedia = buildMedia(containerNode, mediaNode);
            int containerWidth = RegionXlf::width(containerNode);
            int containerHeight = RegionXlf::height(containerNode);

            GetMediaPosition visitor(containerWidth, containerHeight); // FIXME change from visitor
            builtMedia->apply(visitor);

            media.emplace_back(MediaWithPos{std::move(builtMedia), visitor.getMediaX(), visitor.getMediaY()});
        }
    }
    return media;
}

std::vector<std::unique_ptr<IMedia>> MainBuilder::collectInvisibleMedia(const xlf_node& containerNode)
{
    std::vector<std::unique_ptr<IMedia>> media;
    for(auto [nodeName, mediaNode] : containerNode)
    {
        if(nodeName == MediaXlf::NodeName && MediaXlf::type(mediaNode) == MediaXlf::AudioType) // FIXME condition
        {
            media.emplace_back(buildMedia(containerNode, mediaNode));
        }
    }
    return media;
}

std::unique_ptr<IMedia> MainBuilder::buildMedia(const xlf_node& containerNode, const xlf_node& mediaNode)
{
    auto type = MediaXlf::type(mediaNode);

    std::unique_ptr<MediaFactory> factory;

    if(type == MediaXlf::ImageType)
        factory = std::make_unique<ImageFactory>(containerNode, mediaNode);
    else if(type == MediaXlf::VideoType)
        factory = std::make_unique<VideoFactory>(containerNode, mediaNode);
    else if(type == MediaXlf::AudioType)
        factory = std::make_unique<AudioFactory>(containerNode, mediaNode);
    else // NOTE DataSetView, Embedded, Text and Ticker can be rendered via webview
        factory = std::make_unique<WebViewFactory>(containerNode, mediaNode);

    return factory->create();
}
