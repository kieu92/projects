from flask_wtf import FlaskForm
from wtforms import StringField, SubmitField, TextAreaField
from wtforms.validators import InputRequired, Length

class SearchForm(FlaskForm):
    search_query = StringField('Search', validators=[InputRequired(), Length(min=1, max=30)])
    submit = SubmitField('Submit')

class MovieReviewForm(FlaskForm):
    name = StringField('Name', validators=[InputRequired(), Length(min=1, max=50)])
    text = TextAreaField('Text', validators=[InputRequired(), Length(min=1, max=500)])
    submit = SubmitField('Submit')