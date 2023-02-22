
package cmsc433.p5;

import java.io.IOException;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.LongWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.mapreduce.Job;
import org.apache.hadoop.mapreduce.Mapper;
import org.apache.hadoop.mapreduce.Reducer;
import org.apache.hadoop.mapreduce.lib.input.FileInputFormat;
import org.apache.hadoop.mapreduce.lib.output.FileOutputFormat;

/**
 * Map reduce which takes in a CSV file with tweets as input and output
 * key/value pairs.</br>
 * </br>
 * The key for the map reduce depends on the specified {@link TrendingParameter}
 * , <code>trendingOn</code> passed to
 * {@link #score(Job, String, String, TrendingParameter)}).
 */
public class TweetPopularityMR {

	// For your convenience...
	public static final int          TWEET_SCORE   = 1;
	public static final int          RETWEET_SCORE = 3;
	public static final int          MENTION_SCORE = 1;
	public static final int			 PAIR_SCORE = 1;

	// Is either USER, TWEET, HASHTAG, or HASHTAG_PAIR. Set for you before call to map()
	private static TrendingParameter trendingOn;

	public static class TweetMapper
	extends Mapper<LongWritable, Text, Text, IntWritable> {
		
		private final static IntWritable one = new IntWritable(1);
		private final static IntWritable three = new IntWritable(3);

		@Override
		public void map(LongWritable key, Text value, Context context)
				throws IOException, InterruptedException {
			// Converts the CSV line into a tweet object
			Tweet tweet = Tweet.createTweet(value.toString());

			// TODO: Your code goes here
			if (trendingOn == TrendingParameter.USER) {
				String userName = tweet.getUserScreenName();
				String retweetedUser = tweet.getRetweetedUser();
				context.write(new Text(userName), one);
				
				for (String mentionedUser: tweet.getMentionedUsers()) {
					context.write(new Text(mentionedUser), one);
				}
				
				if (tweet.wasRetweetOfUser()) {
					context.write(new Text(retweetedUser), three);
				}
			}
			
			else if (trendingOn == TrendingParameter.TWEET) {
				String tweetId = tweet.getId().toString();
				context.write(new Text(tweetId), one);
				
				if (tweet.wasRetweetOfTweet()) {
					context.write(new Text(tweet.getRetweetedTweet().toString()), three);
				}
			}
			
			else if (trendingOn == TrendingParameter.HASHTAG) {
				for (String hashTag: tweet.getHashtags()) {
					context.write(new Text(hashTag), one);
				}
			}

			else {
				if (tweet.getHashtags().size() > 1) {
					for (int i = 0; i < tweet.getHashtags().size(); i++) {
						for (int j = i + 1; j < tweet.getHashtags().size(); j++) {
							String hashTag1 = tweet.getHashtags().get(i).replace("#", "");
							String hashTag2 = tweet.getHashtags().get(j).replace("#", "");
							String hashTagPair = "";
														
							if (hashTag1.compareTo(hashTag2) > 0) {
								hashTagPair = "(" + hashTag1 + "," + hashTag2 + ")";
							}
							
							else {
								hashTagPair = "(" + hashTag2 + "," + hashTag1 + ")";
							}
							
							context.write(new Text(hashTagPair), new IntWritable(1));
						}
					}
				}
			}
		}
	}

	public static class PopularityReducer
	extends Reducer<Text, IntWritable, Text, IntWritable> {

		@Override
		public void reduce(Text key, Iterable<IntWritable> values, Context context)
				throws IOException, InterruptedException {

			// TODO: Your code goes here
			int frequency = 0;
			
			for (IntWritable value: values) {
				frequency += value.get();
			}

			context.write(key, new IntWritable(frequency));
		}
	}

	/**
	 * Method which performs a map reduce on a specified input CSV file and
	 * outputs the scored tweets, users, or hashtags.</br>
	 * </br>
	 * 
	 * @param job
	 * @param input
	 *          The CSV file containing tweets
	 * @param output
	 *          The output file with the scores
	 * @param trendingOn
	 *          The parameter on which to score
	 * @return true if the map reduce was successful, false otherwise.
	 * @throws Exception
	 */
	public static boolean score(Job job, String input, String output,
			TrendingParameter trendingOn) throws Exception {

		TweetPopularityMR.trendingOn = trendingOn;

		job.setJarByClass(TweetPopularityMR.class);

		// TODO: Set up map-reduce...
		job.setJobName("Tweet Popularity");
		
		job.setOutputKeyClass(Text.class);
		job.setOutputValueClass(IntWritable.class);
		
		job.setMapperClass(TweetMapper.class);
		job.setReducerClass(PopularityReducer.class);
		
		job.setMapOutputKeyClass(Text.class);
		job.setMapOutputValueClass(IntWritable.class);
		// End

		FileInputFormat.addInputPath(job, new Path(input));
		FileOutputFormat.setOutputPath(job, new Path(output));

		return job.waitForCompletion(true);
	}
}
