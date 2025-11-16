async function initializeUI() {
    var nodes = await Vortex.GetNodeTypesAsync();
    // nodes is a dictionary with string : json string pairs
    // print the dictionary to console
    console.log("Node Types:", JSON.stringify(nodes, null, 2));
};

async function testCreateNode() {
    var outputWindow = await Vortex.CreateNodeAsync("WindowOutput");
    Vortex.SetNodePropertyByName(outputWindow, "name", "Test Source 2");
    Vortex.SetNodePropertyByName(outputWindow, "window_size", "[1920,1080]");
    Vortex.SetNodePropertyByName(outputWindow, "framerate", "[30,1]");

    var imageSource = await Vortex.CreateNodeAsync("ImageInput");
    Vortex.SetNodePropertyByName(imageSource, "image_path", "ui/HDR.jpg");


    // Create a ColorCorrection node
    var colorCorrectionNode = await Vortex.CreateNodeAsync("ColorCorrection");
    Vortex.SetNodePropertyByName(colorCorrectionNode, "lut", "C:\\Users\\Agrae\\source\\repos\\Vortex\\rc\\lut.cube");

    await Vortex.ConnectNodesAsync(imageSource, 0, colorCorrectionNode, 0);
    await Vortex.ConnectNodesAsync(colorCorrectionNode, 0, outputWindow, 0);

    Vortex.Play();
}

testCreateNode();