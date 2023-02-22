from flask import Flask, render_template
from model import PokeClient
app = Flask(__name__)

poke_client = PokeClient()

@app.route('/')
def index():
    """
    Must show all of the pokemon names as clickable links

    Check the README for more detail.
    """
    return render_template('index.html', pokes_list=poke_client.get_pokemon_list())

@app.route('/pokemon/<pokemon_name>')
def pokemon_info(pokemon_name):
    """
    Must show all the info for a pokemon identified by name

    Check the README for more detail
    """
    return render_template('poke_info.html', pokes_info=poke_client.get_pokemon_info(pokemon_name))

@app.route('/ability/<ability_name>')
def pokemon_with_ability(ability_name):
    """
    Must show a list of pokemon 

    Check the README for more detail
    """
    return render_template('ability_info.html', ability=ability_name, pokes_with_ability=poke_client.get_pokemon_with_ability(ability_name))
