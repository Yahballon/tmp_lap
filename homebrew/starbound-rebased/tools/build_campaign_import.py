"""Import the CLI authoring script without invoking its command-line entrypoint."""
import importlib.util
from pathlib import Path
spec = importlib.util.spec_from_file_location('campaign_author', Path(__file__).with_name('build-campaign.py'))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
build = module.build
LAYERS = module.LAYERS
