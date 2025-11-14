async function initializeUI() {
    var nodes = await Vortex.GetNodeTypesAsync();
    // nodes is a dictionary with string : json string pairs
    // print the dictionary to console
    console.log("Node Types:", JSON.stringify(nodes, null, 2));
};

async function testCreateNode() {
    var outputNDI = await Vortex.CreateNodeAsync("NDIOutput");
    var outputWindow = await Vortex.CreateNodeAsync("WindowOutput");

    console.log("Created NDI Output Node with ID:", outputNDI);

    Vortex.SetNodePropertyByName(outputNDI, "name", "Test Source");
    Vortex.SetNodePropertyByName(outputNDI, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputNDI, "framerate", "[30,1]");

    Vortex.SetNodePropertyByName(outputWindow, "name", "Test Source 2");
    Vortex.SetNodePropertyByName(outputWindow, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputWindow, "framerate", "[30,1]");

    var imageSource = await Vortex.CreateNodeAsync("ImageInput");
    Vortex.SetNodePropertyByName(imageSource, "image_path", "ui/HDR.jpg");

    await Vortex.ConnectNodesAsync(imageSource, 0, outputNDI, 0);
    await Vortex.ConnectNodesAsync(imageSource, 0, outputWindow, 0);
    Vortex.Play();
}

testCreateNode();