async function initializeUI() {
    var nodes = await Vortex.GetNodeTypesAsync();
    // nodes is a dictionary with string : json string pairs
    // print the dictionary to console
    console.log("Node Types:", JSON.stringify(nodes, null, 2));
};

async function testCreateNode() {
    let outputWindow = await Vortex.CreateNodeAsync2("WindowOutput");
}

testCreateNode();